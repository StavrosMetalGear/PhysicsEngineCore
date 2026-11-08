#include "PhysicsEngine.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

namespace fs = std::filesystem;

class TemporaryDirectory {
public:
	TemporaryDirectory()
	{
		std::random_device random;
		const auto root = fs::temp_directory_path();
		for (int attempt = 0; attempt < 100; ++attempt) {
			auto candidate = root / ("physics-engine-quantum-exports-" + std::to_string(random()) + "-" + std::to_string(random()));
			std::error_code error;
			if (fs::create_directory(candidate, error)) {
				path = candidate;
				return;
			}
			if (error && error != std::errc::file_exists) {
				throw fs::filesystem_error("Could not create temporary directory", candidate, error);
			}
		}
		throw std::runtime_error("Could not allocate a unique temporary directory");
	}

	~TemporaryDirectory()
	{
		std::error_code error;
		fs::remove_all(path, error);
	}

	fs::path path;
};

bool verifyCsv(const fs::path& path, const std::string& header,
			   const std::function<std::vector<double>(int)>& expected)
{
	std::ifstream input(path);
	input.imbue(std::locale::classic());
	std::string line;
	if (!std::getline(input, line) || line != header) {
		std::cerr << "FAIL: missing file or unexpected CSV header: " << path << "\n";
		return false;
	}
	for (int row = 0; row < 3; ++row) {
		if (!std::getline(input, line)) {
			std::cerr << "FAIL: missing CSV row " << row << ": " << path << "\n";
			return false;
		}
		std::istringstream fields(line);
		fields.imbue(std::locale::classic());
		const auto values = expected(row);
		std::string field;
		for (std::size_t column = 0; column < values.size(); ++column) {
			if (!std::getline(fields, field, ',')) {
				std::cerr << "FAIL: missing CSV column: " << path << "\n";
				return false;
			}
			std::istringstream number(field);
			number.imbue(std::locale::classic());
			double actual = 0.0;
			if (!(number >> actual) || !(number >> std::ws).eof() || !std::isfinite(actual) ||
				std::abs(actual - values[column]) > 1.0e-5 * std::max(std::abs(values[column]), 1.0e-300)) {
				std::cerr << "FAIL: invalid CSV value in " << path << " row " << row
						  << " column " << column << ": " << field << " expected " << values[column] << "\n";
				return false;
			}
		}
		if (std::getline(fields, field, ',') || (!line.empty() && line.back() == ',')) {
			std::cerr << "FAIL: extra CSV column: " << path << "\n";
			return false;
		}
	}
	if (std::getline(input, line) || input.bad()) {
		std::cerr << "FAIL: extra row or read error: " << path << "\n";
		return false;
	}
	return true;
}

bool testExports(const fs::path& root)
{
	constexpr double pi = 3.14159265358979323846;
	constexpr double hbar = 1.0545718e-34;
	constexpr double c = 299792458.0;
	QP_Handle quantum = qp_create("quantum-csv", 9.1093837015e-31, 1.0e-10, 1);
	if (!quantum) {
		std::cerr << "FAIL: Quantum handle creation\n";
		return false;
	}
	struct HandleOwner {
		QP_Handle handle;
		~HandleOwner() { qp_destroy(handle); }
	} owner{quantum};
	bool ok = true;
	auto filename = [&](const char* name) { return (root / name).string(); };
	auto verify = [&](const char* name, const char* header, const std::function<std::vector<double>(int)>& expected) {
		if (!verifyCsv(root / name, header, expected)) ok = false;
	};

	qp_export_rabi_csv(quantum, filename("rabi.csv").c_str(), 2.0, 0.0, pi / 2.0, 2);
	verify("rabi.csv", "t,P_rabi", [&](int i) {
		const double t = i * pi / 4.0;
		return std::vector<double>{t, std::pow(std::sin(t), 2.0)};
	});
	qp_export_transition_prob_csv(quantum, filename("transition.csv").c_str(), 0.1 * hbar, 2.0, 2.0, 2.0, 2);
	verify("transition.csv", "t,P_transition", [&](int i) {
		const double t = static_cast<double>(i);
		return std::vector<double>{t, 0.01 * t * t};
	});
	qp_export_berry_phase_csv(quantum, filename("berry.csv").c_str(), 2);
	verify("berry.csv", "theta,gamma_berry,solid_angle,gamma_over_neg_half_omega", [&](int i) {
		const double theta = i * pi / 2.0;
		const double solidAngle = 2.0 * pi * (1.0 - std::cos(theta));
		return std::vector<double>{theta, -solidAngle / 2.0, solidAngle, 1.0};
	});
	qp_export_landau_zener_csv(quantum, filename("landau-zener.csv").c_str(), 1.0e-20, 3.0e-5, 3);
	verify("landau-zener.csv", "alpha,P_diabatic,P_adiabatic", [&](int i) {
		const double alpha = (i + 1) * 1.0e-5;
		const double probability = std::exp(-2.0 * pi * 1.0e-40 / (hbar * alpha));
		return std::vector<double>{alpha, probability, 1.0 - probability};
	});
	qp_export_tight_binding_dispersion_csv(quantum, filename("tight-binding.csv").c_str(), 5.0, 2.0, 1.0, 3);
	verify("tight-binding.csv", "k,E", [&](int i) {
		const double k = (i - 1) * pi;
		return std::vector<double>{k, 5.0 - 4.0 * std::cos(k)};
	});
	qp_export_relativistic_dispersion_csv(quantum, filename("relativistic.csv").c_str(), 1.0, 2.0 * c, 2);
	verify("relativistic.csv", "p,E_relativistic,E_nonrelativistic,T_relativistic,T_nonrelativistic", [&](int i) {
		const double p = i * c;
		const double rest = c * c;
		const double relativistic = std::sqrt(p * p * c * c + rest * rest);
		const double kinetic = p * p / 2.0;
		return std::vector<double>{p, relativistic, rest + kinetic, relativistic - rest, kinetic};
	});
	return ok;
}

}

int main()
{
	try {
		TemporaryDirectory temporary;
		return testExports(temporary.path) ? 0 : 1;
	} catch (const std::exception& error) {
		std::cerr << "FAIL: " << error.what() << "\n";
		return 1;
	}
}
