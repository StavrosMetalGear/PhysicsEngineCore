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
			auto candidate = root / ("physics-engine-exports-" + std::to_string(random()) + "-" + std::to_string(random()));
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
				std::abs(actual - values[column]) > 1.0e-5 * std::max(std::abs(values[column]), 1.0e-25)) {
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
	constexpr double eps0 = 8.854187817e-12;
	constexpr double mu0 = 1.2566370614e-6;
	constexpr double ke = 8.9875517873681764e9;
	constexpr double charge = 1.0e-9;
	EM_Handle em = em_create("csv-exports", 2.5, 1.0, 5.8e7);
	if (!em) {
		std::cerr << "FAIL: Electrodynamics handle creation\n";
		return false;
	}
	struct HandleOwner {
		EM_Handle handle;
		~HandleOwner() { em_destroy(handle); }
	} owner{em};
	bool ok = true;
	auto verify = [&](const char* name, const char* header, const std::function<std::vector<double>(int)>& expected) {
		if (!verifyCsv(root / name, header, expected)) ok = false;
	};
	auto filename = [&](const char* name) { return (root / name).string(); };

	em_export_coulomb_csv(em, filename("coulomb.csv").c_str(), charge, 3);
	verify("coulomb.csv", "r,E,V,U(q1=q)", [&](int i) {
		const double r = 0.01 + i;
		return std::vector<double>{r, ke * charge / (r * r), ke * charge / r, ke * charge * charge / r};
	});
	em_export_gauss_csv(em, filename("gauss.csv").c_str(), charge, 0.2, 3);
	verify("gauss.csv", "r,E", [&](int i) {
		const double r = 0.001 + 0.2 * i;
		const double e = r < 0.2 ? charge * r / (4.0 * pi * 2.5 * eps0 * 0.008)
								  : charge / (4.0 * pi * 2.5 * eps0 * r * r);
		return std::vector<double>{r, e};
	});
	em_export_potential_csv(em, filename("potential.csv").c_str(), charge, 0.2, 3);
	verify("potential.csv", "z,V_point,V_ring,V_shell", [&](int i) {
		const double z = 0.001 + 0.25 * i;
		return std::vector<double>{z, ke * charge / z, ke * charge / std::sqrt(0.04 + z * z),
								   ke * charge / (z < 0.2 ? 0.2 : z)};
	});
	em_export_capacitance_csv(em, filename("capacitance.csv").c_str(), 0.02, 3);
	verify("capacitance.csv", "d,C", [&](int i) {
		const double d = 0.0001 + 0.005 * i;
		return std::vector<double>{d, 2.5 * eps0 * 0.02 / d};
	});
	em_export_dielectric_csv(em, filename("dielectric.csv").c_str(), 100.0, 3);
	verify("dielectric.csv", "chi_e,P,D", [&](int i) {
		const double chi = 0.01 + 5.0 * i;
		return std::vector<double>{chi, eps0 * chi * 100.0, 2.5 * eps0 * 100.0};
	});
	em_export_resistance_csv(em, filename("resistance.csv").c_str(), 2.0e-6, 3);
	verify("resistance.csv", "L,R", [](int i) {
		const double length = 0.01 + 5.0 * i;
		return std::vector<double>{length, 2.0 * length};
	});
	em_export_rc_csv(em, filename("rc.csv").c_str(), 12.0, 1000.0, 2.0e-6, 3);
	verify("rc.csv", "t,V_charge,I_discharge", [](int i) {
		const double time = 0.005 * i;
		return std::vector<double>{time, 12.0 * (1.0 - std::exp(-time / 0.002)),
								   0.012 * std::exp(-time / 0.002)};
	});
	em_export_biot_savart_csv(em, filename("biot.csv").c_str(), 4.0, 0.2, 3);
	verify("biot.csv", "z,B_on_axis", [&](int i) {
		const double z = -0.5 + 0.5 * i;
		return std::vector<double>{z, mu0 * 4.0 * 0.04 / (2.0 * std::pow(0.04 + z * z, 1.5))};
	});
	em_export_ampere_csv(em, filename("ampere.csv").c_str(), 4.0, 0.2, 3);
	verify("ampere.csv", "r,B", [&](int i) {
		const double r = 0.0001 + 0.3 * i;
		const double b = r < 0.2 ? mu0 * 4.0 * r / (2.0 * pi * 0.04)
								  : mu0 * 4.0 / (2.0 * pi * r);
		return std::vector<double>{r, b};
	});
	em_export_faraday_csv(em, filename("faraday.csv").c_str(), 2.0, 0.5, 3);
	verify("faraday.csv", "v,EMF", [](int i) {
		const double v = 50.0 * i;
		return std::vector<double>{v, v};
	});
	return ok;
}

}

int main()
{
	try {
		TemporaryDirectory directory;
		return testExports(directory.path) ? 0 : 1;
	} catch (const std::exception& error) {
		std::cerr << "FAIL: CSV export test setup: " << error.what() << "\n";
		return 1;
	}
}
