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
			auto candidate = root / ("physics-engine-nuclear-exports-" + std::to_string(random()) + "-" + std::to_string(random()));
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

bool verifyCsv(const fs::path& path, const std::string& header, int rowCount,
			   const std::function<std::vector<double>(int)>& expected)
{
	std::ifstream input(path);
	input.imbue(std::locale::classic());
	std::string line;
	if (!std::getline(input, line) || line != header) {
		std::cerr << "FAIL: missing file or unexpected CSV header: " << path << "\n";
		return false;
	}
	for (int row = 0; row < rowCount; ++row) {
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
	constexpr double electronMass = 0.510998950;
	NP_Handle nuclear = np_create();
	if (!nuclear) {
		std::cerr << "FAIL: Nuclear handle creation\n";
		return false;
	}
	struct HandleOwner {
		NP_Handle handle;
		~HandleOwner() { np_destroy(handle); }
	} owner{nuclear};
	bool ok = true;
	auto filename = [&](const char* name) { return (root / name).string(); };
	auto verify = [&](const char* name, const char* header, int rows, const std::function<std::vector<double>(int)>& expected) {
		if (!verifyCsv(root / name, header, rows, expected)) ok = false;
	};

	const auto bindingReference = [&](int row) {
		const int a = row + 1;
		const double a13 = std::cbrt(static_cast<double>(a));
		const int z = std::max(1, std::min(a, static_cast<int>(std::round(a / (2.0 + 0.7 * a13 * a13 / (2.0 * 23.285))))));
		const int n = a - z;
		const double pairing = a % 2 == 0 ? (z % 2 == 0 ? 12.0 : -12.0) / std::sqrt(static_cast<double>(a)) : 0.0;
		const double energy = 15.56 * a - 17.23 * a13 * a13 - 0.7 * z * (z - 1) / a13
			- 23.285 * (n - z) * (n - z) / a + pairing;
		return std::vector<double>{static_cast<double>(a), std::max(energy, 0.0) / a};
	};
	np_export_semf_csv(nuclear, filename("semf.csv").c_str(), 56);
	verify("semf.csv", "A,B_per_A_MeV", 56, bindingReference);
	np_export_binding_curve_csv(nuclear, filename("binding.csv").c_str());
	verify("binding.csv", "A,B_per_A_MeV", 260, bindingReference);

	np_export_rutherford_csv(nuclear, filename("rutherford.csv").c_str(), 2, 79, 5.0);
	verify("rutherford.csv", "theta_deg,dSigma_dOmega_fm2", 197, [&](int row) {
		const double angle = (row + 4) * pi / 200.0;
		const double length = 2.0 * 79.0 * 1.4399764 / (4.0 * 5.0);
		return std::vector<double>{(row + 4) * 0.9, length * length / std::pow(std::sin(angle / 2.0), 4)};
	});

	np_export_beta_decay_csv(nuclear, filename("beta.csv").c_str(), 2, 1.0);
	verify("beta.csv", "Te_MeV,dN_dTe", 199, [&](int row) {
		const double kinetic = (row + 1) / 200.0;
		const double energy = kinetic + electronMass;
		const double momentum = std::sqrt(kinetic * (kinetic + 2.0 * electronMass));
		const double eta = 2.0 * energy / (137.035999084 * momentum);
		const double fermi = 2.0 * pi * eta / (-std::expm1(-2.0 * pi * eta));
		const double spectrum = fermi * momentum * energy * (1.0 - kinetic) * (1.0 - kinetic) / std::pow(electronMass, 4);
		return std::vector<double>{kinetic, spectrum};
	});

	const double projectileMass = 938.272046;
	np_export_bethe_bloch_csv(nuclear, filename("bethe-bloch.csv").c_str(), 1, projectileMass, 13, 26.98);
	verify("bethe-bloch.csv", "T_MeV,dEdx_MeVcm2g", 300, [&](int row) {
		const double energy = 0.1 + 10000.0 * (row + 1) / 300.0;
		const double gamma = 1.0 + energy / projectileMass;
		const double betaSquared = 1.0 - 1.0 / (gamma * gamma);
		const double ratio = electronMass / projectileMass;
		const double maximumTransfer = 2.0 * electronMass * betaSquared * gamma * gamma /
			(1.0 + 2.0 * gamma * ratio + ratio * ratio);
		const double argument = 2.0 * electronMass * betaSquared * gamma * gamma * maximumTransfer / (78.0e-6 * 78.0e-6);
		const double stopping = 0.307075 * 13.0 / (26.98 * betaSquared) * (0.5 * std::log(argument) - betaSquared);
		return std::vector<double>{energy, stopping};
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
