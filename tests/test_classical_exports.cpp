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
			auto candidate = root / ("physics-engine-classical-exports-" + std::to_string(random()) + "-" + std::to_string(random()));
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
				std::abs(actual - values[column]) > 1.0e-12 + 1.0e-5 * std::abs(values[column])) {
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
	constexpr double mass = 2.0;
	CP_Handle classical = cp_create("classical-csv", mass, 1.0, 1);
	if (!classical) {
		std::cerr << "FAIL: Classical handle creation\n";
		return false;
	}
	struct HandleOwner {
		CP_Handle handle;
		~HandleOwner() { cp_destroy(handle); }
	} owner{classical};
	bool ok = true;
	auto filename = [&](const char* name) { return (root / name).string(); };
	auto verify = [&](const char* name, const char* header, const std::function<std::vector<double>(int)>& expected, int rows = 3) {
		if (!verifyCsv(root / name, header, rows, expected)) ok = false;
	};

	cp_exportFreeParticleCSV(classical, filename("free.csv").c_str(), 2.0, 3.0, 2.0, 2);
	verify("free.csv", "t,x,v,KE,p", [&](int i) {
		const double t = static_cast<double>(i);
		return std::vector<double>{t, 2.0 + 3.0 * t, 3.0, 9.0, 6.0};
	});
	cp_exportProjectileCSV(classical, filename("projectile.csv").c_str(), 10.0, pi / 6.0, 10.0, 2);
	verify("projectile.csv", "t,x,y,vx,vy", [&](int i) {
		const double t = i / 2.0;
		return std::vector<double>{t, 5.0 * std::sqrt(3.0) * t, 5.0 * t - 5.0 * t * t,
			5.0 * std::sqrt(3.0), 5.0 - 10.0 * t};
	});
	cp_exportProjectileWithDragCSV(classical, filename("projectile-drag.csv").c_str(), 10.0, pi / 6.0, 10.0, 0.0, 0.1, 2);
	verify("projectile-drag.csv", "t,x,y,vx,vy", [&](int i) {
		const double t = i * 0.1;
		return std::vector<double>{t, 5.0 * std::sqrt(3.0) * t, i * 0.5 - i * (i + 1) * 0.05,
			5.0 * std::sqrt(3.0), 5.0 - i};
	});
	cp_exportSHOCSV(classical, filename("sho.csv").c_str(), 2.0, 1.0, 0.3, 2.0, 2);
	verify("sho.csv", "t,x,v,KE,PE,E", [&](int i) {
		const double t = static_cast<double>(i);
		const double x = 2.0 * std::cos(t + 0.3);
		const double v = -2.0 * std::sin(t + 0.3);
		return std::vector<double>{t, x, v, v * v, x * x, 4.0};
	});
	cp_exportSHOPhaseSpaceCSV(classical, filename("sho-phase.csv").c_str(), 2.0, 1.0, 0.3, 2.0, 2);
	verify("sho-phase.csv", "x,p", [&](int i) {
		return std::vector<double>{2.0 * std::cos(i + 0.3), -4.0 * std::sin(i + 0.3)};
	});
	cp_exportDampedHOCSV(classical, filename("damped.csv").c_str(), 2.0, std::sqrt(5.0), 1.0, 2.0, 2);
	verify("damped.csv", "t,x,envelope", [&](int i) {
		const double t = static_cast<double>(i);
		const double envelope = 2.0 * std::exp(-t);
		return std::vector<double>{t, envelope * std::cos(2.0 * t), envelope};
	});
	cp_exportKeplerEffectivePotentialCSV(classical, filename("kepler-potential.csv").c_str(), 2.0, 1.0, 1.0, 1.0, 4.0, 3);
	verify("kepler-potential.csv", "r,Veff", [&](int i) {
		const double r = i + 2.0;
		return std::vector<double>{r, 2.0 / (r * r) - 1.0 / r};
	});
	cp_exportDrivenHOResonanceCSV(classical, filename("driven-resonance.csv").c_str(), 4.0, 2.0, 0.5, 0.0, 2.0, 2);
	verify("driven-resonance.csv", "omega,amplitude,phase,power", [&](int i) {
		const double omega = static_cast<double>(i);
		const double difference = 4.0 - omega * omega;
		const double denominator = difference * difference + omega * omega;
		return std::vector<double>{omega, 2.0 / std::sqrt(denominator),
			std::atan2(omega, difference), 2.0 * omega * omega / denominator};
	});
	cp_exportDrivenHOTimeCSV(classical, filename("driven-time.csv").c_str(), 4.0, 2.0, 1.0, 0.5, 2.0, 2);
	verify("driven-time.csv", "t,x_steady,driving_force", [&](int i) {
		const double t = static_cast<double>(i);
		return std::vector<double>{t, 2.0 / std::sqrt(10.0) * std::cos(t - std::atan2(1.0, 3.0)),
			2.0 * std::cos(t)};
	});
	cp_exportCoupledOscillatorsCSV(classical, filename("coupled.csv").c_str(), 2.0, 2.0, 1.0, 2.0, 2.0,
		1.0, -1.0, 0.0, 0.0, 0.2, 2);
	verify("coupled.csv", "t,x1,x2,v1,v2", [&](int i) {
		const double t = i * 0.1;
		const double x = std::cos(std::sqrt(2.0) * t);
		const double v = -std::sqrt(2.0) * std::sin(std::sqrt(2.0) * t);
		return std::vector<double>{t, x, -x, v, -v};
	});
	cp_exportKeplerOrbitCSV(classical, filename("kepler-orbit.csv").c_str(), -0.375, 1.0, 1.0, 1.0, 2);
	verify("kepler-orbit.csv", "theta,r,x,y", [&](int i) {
		const double theta = pi * i;
		const double r = 1.0 / (1.0 + 0.5 * std::cos(theta));
		return std::vector<double>{theta, r, r * std::cos(theta), r * std::sin(theta)};
	});
	const double gravitationalConstant = 6.67430e-11;
	const double bodyMass = 1.0 / (2.0 * gravitationalConstant);
	cp_exportTwoBodyOrbitCSV(classical, filename("two-body.csv").c_str(), bodyMass, bodyMass, 1.0, 0.0, 0.2, 2);
	verify("two-body.csv", "t,x,y,vx,vy,r,E", [&](int i) {
		const double x1 = 0.995;
		const double v1 = -0.05 * (1.0 + 1.0 / (x1 * x1));
		const double x2 = x1 + 0.1 * v1 - 0.005 / (x1 * x1);
		const double v2 = v1 - 0.05 * (1.0 / (x1 * x1) + 1.0 / (x2 * x2));
		const double x = i == 0 ? 1.0 : (i == 1 ? x1 : x2);
		const double v = i == 0 ? 0.0 : (i == 1 ? v1 : v2);
		const double energy = bodyMass / 4.0 * v * v - gravitationalConstant * bodyMass * bodyMass / x;
		return std::vector<double>{i * 0.1, x, 0.0, v, 0.0, x, energy};
	});
	cp_exportChargedParticleTrajectoryCSV(classical, filename("charged.csv").c_str(), 2.0, 2.0,
		1.0, 2.0, 3.0, 1.0, -2.0, 3.0, 2.0, -1.0, 0.5, 0.0, 0.0, 0.0, 1.0, 2);
	verify("charged.csv", "t,x,y,z,vx,vy,vz", [&](int i) {
		const double t = i * 0.5;
		const double displacementFactor = 0.125 * i * (i + 1);
		return std::vector<double>{t, 1.0 + t + 2.0 * displacementFactor,
			2.0 - 2.0 * t - displacementFactor, 3.0 + 3.0 * t + 0.5 * displacementFactor,
			1.0 + 2.0 * t, -2.0 - t, 3.0 + 0.5 * t};
	});
	const double bodies[] = {
		-0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0,
		0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0
	};
	cp_exportNBodyCSV(classical, filename("nbody.csv").c_str(), bodies, 2, 1.0, 0.1, 2);
	verify("nbody.csv", "step,body,x,y,z,vx,vy,vz", [&](int row) {
		const int step = row / 2;
		const int body = row % 2;
		const double sign = body == 0 ? 1.0 : -1.0;
		const double velocity = step == 0 ? 0.0 : (step == 1 ? 0.1 : 0.1 + 0.1 / (0.98 * 0.98));
		const double position = step == 0 ? -0.5 : (step == 1 ? -0.49 : -0.49 + 0.1 * velocity);
		return std::vector<double>{static_cast<double>(step), static_cast<double>(body), sign * position,
			0.0, 0.0, sign * velocity, 0.0, 0.0};
	}, 6);
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
