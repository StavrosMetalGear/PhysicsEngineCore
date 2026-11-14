#include "PhysicsEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
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

bool verifyCsv(const fs::path& path, const std::string& header, int rowCount,
			   const std::function<std::vector<double>(int)>& expected,
			   const std::vector<double>& absoluteTolerances = {})
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
			const double absoluteTolerance = column < absoluteTolerances.size() ? absoluteTolerances[column] : 0.0;
			if (!(number >> actual) || !(number >> std::ws).eof() || !std::isfinite(actual) ||
				std::abs(actual - values[column]) > absoluteTolerance + 1.0e-5 * std::max(std::abs(values[column]), 1.0e-300)) {
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

bool verifyLabeledCsv(const fs::path& path, const std::string& header,
					  const std::vector<std::pair<std::string, std::vector<double>>>& expected)
{
	std::ifstream input(path);
	input.imbue(std::locale::classic());
	std::string line;
	if (!std::getline(input, line) || line != header) {
		std::cerr << "FAIL: missing file or unexpected CSV header: " << path << "\n";
		return false;
	}
	for (const auto& row : expected) {
		if (!std::getline(input, line)) {
			std::cerr << "FAIL: missing labeled CSV row: " << path << "\n";
			return false;
		}
		std::istringstream fields(line);
		fields.imbue(std::locale::classic());
		std::string field;
		if (!std::getline(fields, field, ',') || field != row.first) {
			std::cerr << "FAIL: unexpected CSV label: " << path << "\n";
			return false;
		}
		for (double value : row.second) {
			if (!std::getline(fields, field, ',')) {
				std::cerr << "FAIL: missing labeled CSV column: " << path << "\n";
				return false;
			}
			std::istringstream number(field);
			number.imbue(std::locale::classic());
			double actual = 0.0;
			if (!(number >> actual) || !(number >> std::ws).eof() || !std::isfinite(actual) ||
				std::abs(actual - value) > 1.0e-5 * std::max(std::abs(value), 1.0e-300)) {
				std::cerr << "FAIL: invalid labeled CSV value: " << path << " label " << row.first
						  << " value " << field << " expected " << value << "\n";
				return false;
			}
		}
		if (std::getline(fields, field, ',') || (!line.empty() && line.back() == ',')) {
			std::cerr << "FAIL: extra labeled CSV column: " << path << "\n";
			return false;
		}
	}
	if (std::getline(input, line) || input.bad()) {
		std::cerr << "FAIL: extra labeled CSV row or read error: " << path << "\n";
		return false;
	}
	return true;
}

bool verifyBoxLevels(const fs::path& path, int dimensions, double groundUnit)
{
	std::ifstream input(path);
	input.imbue(std::locale::classic());
	std::string line;
	const char* header = dimensions == 2 ? "nx,ny,E,nx2_plus_ny2" : "nx,ny,nz,E,n2_sum";
	if (!std::getline(input, line) || line != header) {
		std::cerr << "FAIL: missing file or unexpected box-level header: " << path << "\n";
		return false;
	}
	std::set<std::array<int, 3>> states;
	const int expectedRows = dimensions == 2 ? 4 : 8;
	int previousSum = 0;
	for (int row = 0; row < expectedRows; ++row) {
		if (!std::getline(input, line)) {
			std::cerr << "FAIL: missing box-level row: " << path << "\n";
			return false;
		}
		std::istringstream fields(line);
		fields.imbue(std::locale::classic());
		std::array<int, 3> state{};
		char separator = 0;
		for (int axis = 0; axis < dimensions; ++axis) {
			if (!(fields >> state[axis] >> separator) || separator != ',' || state[axis] < 1 || state[axis] > 2) {
				std::cerr << "FAIL: invalid box-level state: " << path << "\n";
				return false;
			}
		}
		double energy = 0.0;
		int sum = 0;
		const int expectedSum = state[0] * state[0] + state[1] * state[1] + state[2] * state[2];
		if (!(fields >> energy >> separator >> sum) || separator != ',' || !(fields >> std::ws).eof() ||
			!std::isfinite(energy) || sum != expectedSum || sum < previousSum ||
			std::abs(energy - groundUnit * sum) > 1.0e-5 * groundUnit * sum || !states.insert(state).second) {
			std::cerr << "FAIL: invalid, unordered or duplicate box level: " << path << "\n";
			return false;
		}
		previousSum = sum;
	}
	if (std::getline(input, line) || input.bad()) {
		std::cerr << "FAIL: extra box-level row or read error: " << path << "\n";
		return false;
	}
	return true;
}

bool verifyCoherentStateCsv(const fs::path& path, double mass, double omega, double hbar, double pi)
{
	std::ifstream input(path);
	input.imbue(std::locale::classic());
	std::string line;
	auto header = [&](const char* expected) {
		return std::getline(input, line) && line == expected;
	};
	auto values = [&](double expectedFirst, double expectedSecond) {
		if (!std::getline(input, line)) return false;
		std::istringstream row(line);
		row.imbue(std::locale::classic());
		double first = 0.0, second = 0.0;
		char comma = 0;
		return (row >> first >> comma >> second) && comma == ',' && (row >> std::ws).eof() &&
			std::isfinite(first) && std::isfinite(second) &&
			std::abs(first - expectedFirst) <= 1.0e-5 * std::max(std::abs(expectedFirst), 1.0e-300) &&
			std::abs(second - expectedSecond) <= 1.0e-5 * std::max(std::abs(expectedSecond), 1.0e-300);
	};
	const double positionScale = std::sqrt(hbar / (mass * omega));
	const double prefactor = std::pow(mass * omega / (pi * hbar), 0.25);
	const double poisson = std::exp(-1.0);
	if (!header("n,P_n") || !values(0.0, poisson) || !values(1.0, poisson) ||
		!values(2.0, poisson / 2.0) || !header("") || !header("x,psi") ||
		!values(-5.0 * positionScale, prefactor * std::exp(-std::pow(-5.0 - std::sqrt(2.0), 2.0) / 2.0)) ||
		!values(0.0, prefactor * std::exp(-1.0)) || std::getline(input, line) || input.bad()) {
		std::cerr << "FAIL: invalid coherent-state CSV: " << path << "\n";
		return false;
	}
	return true;
}

bool verifyTwoQubitCsv(const fs::path& path)
{
	std::ifstream input(path);
	input.imbue(std::locale::classic());
	std::string line;
	const double s = 1.0 / std::sqrt(2.0);
	const std::vector<std::pair<std::string, std::vector<double>>> rows = {
		{"|00>", {s, 0.0, 0.5}}, {"|01>", {0.0, 0.0, 0.0}},
		{"|10>", {0.0, 0.0, 0.0}}, {"|11>", {0.0, s, 0.5}},
		{"concurrence", {1.0}}, {"entropy_A", {std::log(2.0)}},
		{"P(A=0)", {0.5}}, {"P(A=1)", {0.5}}, {"P(B=0)", {0.5}}, {"P(B=1)", {0.5}}
	};
	auto header = [&](const char* text) { return std::getline(input, line) && line == text; };
	if (!header("basis,Re,Im,prob")) return false;
	for (std::size_t i = 0; i < rows.size(); ++i) {
		if (i == 4 && (!header("") || !header("property,value"))) return false;
		if (!std::getline(input, line)) return false;
		std::istringstream fields(line);
		fields.imbue(std::locale::classic());
		std::string field;
		if (!std::getline(fields, field, ',') || field != rows[i].first) return false;
		for (double expected : rows[i].second) {
			if (!std::getline(fields, field, ',')) return false;
			std::istringstream number(field);
			number.imbue(std::locale::classic());
			double actual = 0.0;
			if (!(number >> actual) || !(number >> std::ws).eof() || !std::isfinite(actual) ||
				std::abs(actual - expected) > 1.0e-5 * std::max(std::abs(expected), 1.0e-300)) return false;
		}
		if (std::getline(fields, field, ',') || (!line.empty() && line.back() == ',')) return false;
	}
	return !std::getline(input, line) && !input.bad();
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
	auto verify = [&](const char* name, const char* header, const std::function<std::vector<double>(int)>& expected, int rows = 3) {
		if (!verifyCsv(root / name, header, rows, expected)) ok = false;
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
	qp_export_ab_interference_csv(quantum, filename("ab-interference.csv").c_str(), 0.0, 1.0, 0.0, 3);
	verify("ab-interference.csv", "theta,I_with_flux,I_no_flux", [&](int i) {
		return std::vector<double>{(i - 1) * pi / 4.0, 1.0, 1.0};
	});
	qp_export_ab_scattering_csv(quantum, filename("ab-scattering.csv").c_str(), 2.0, 0.5, 4);
	verify("ab-scattering.csv", "theta,dsigma", [&](int i) {
		const double theta = (i + 1) * pi / 4.0;
		return std::vector<double>{theta, 1.0 / (4.0 * pi * std::pow(std::sin(theta / 2.0), 2.0))};
	});
	qp_export_ab_flux_sweep_csv(quantum, filename("ab-flux.csv").c_str(), 0.0, 1.0, 0.0, 2);
	verify("ab-flux.csv", "flux_ratio,I,phase_rad", [&](int i) {
		return std::vector<double>{static_cast<double>(i), 1.0, 2.0 * pi * i};
	});
	const double mass = 9.1093837015e-31;
	qp_export_wavefunction_csv(quantum, filename("box-1d-wave.csv").c_str(), 2, 5);
	verify("box-1d-wave.csv", "x,psi", [&](int i) {
		const double x = i * 0.2e-10;
		return std::vector<double>{x, std::sqrt(2.0e10) * std::sin(2.0 * i * pi / 5.0)};
	}, 5);
	qp_export_two_particle_isw_csv(quantum, filename("two-particle-box.csv").c_str(), 1, 2, 4);
	if (!verifyCsv(root / "two-particle-box.csv", "x1,x2,psi_symmetric,psi_antisymmetric,prob_sym,prob_antisym", 16, [&](int row) {
		const double x1 = (row / 4) * 1.0e-10 / 3.0;
		const double x2 = (row % 4) * 1.0e-10 / 3.0;
		const double a = std::sin(pi * x1 / 1.0e-10) * std::sin(2.0 * pi * x2 / 1.0e-10);
		const double b = std::sin(pi * x2 / 1.0e-10) * std::sin(2.0 * pi * x1 / 1.0e-10);
		const double symmetric = std::sqrt(2.0) * (a + b) / 1.0e-10;
		const double antisymmetric = std::sqrt(2.0) * (a - b) / 1.0e-10;
		return std::vector<double>{x1, x2, symmetric, antisymmetric,
			symmetric * symmetric, antisymmetric * antisymmetric};
	}, {0.0, 0.0, 1.0e-4, 1.0e-4, 1.0e-8, 1.0e-8})) ok = false;
	qp_export_wavefunction_2d_box_csv(quantum, filename("box-2d-wave.csv").c_str(), 1, 1, 2.0, 2.0, 2);
	verify("box-2d-wave.csv", "x,y,psi", [&](int row) {
		const double x = static_cast<double>(row / 3);
		const double y = static_cast<double>(row % 3);
		return std::vector<double>{x, y, std::sin(pi * x / 2.0) * std::sin(pi * y / 2.0)};
	}, 9);
	qp_export_wavefunction_3d_box_slice_csv(quantum, filename("box-3d-wave.csv").c_str(),
		1, 1, 1, 2.0, 2.0, 2.0, 1.0, 2);
	verify("box-3d-wave.csv", "x,y,psi", [&](int row) {
		const double x = static_cast<double>(row / 3);
		const double y = static_cast<double>(row % 3);
		return std::vector<double>{x, y, std::sin(pi * x / 2.0) * std::sin(pi * y / 2.0)};
	}, 9);
	const double boxLength = 1.0e-9;
	const double boxEnergy = hbar * hbar * pi * pi / (2.0 * mass * boxLength * boxLength);
	qp_export_energy_levels_2d_box_csv(quantum, filename("box-2d-levels.csv").c_str(), boxLength, 1);
	verify("box-2d-levels.csv", "nx,ny,E,nx2_plus_ny2", [&](int) {
		return std::vector<double>{1.0, 1.0, 2.0 * boxEnergy, 2.0};
	}, 1);
	qp_export_energy_levels_3d_box_csv(quantum, filename("box-3d-levels.csv").c_str(), boxLength, 1);
	verify("box-3d-levels.csv", "nx,ny,nz,E,n2_sum", [&](int) {
		return std::vector<double>{1.0, 1.0, 1.0, 3.0 * boxEnergy, 3.0};
	}, 1);
	qp_export_energy_levels_2d_box_csv(quantum, filename("box-2d-degenerate.csv").c_str(), boxLength, 2);
	if (!verifyBoxLevels(root / "box-2d-degenerate.csv", 2, boxEnergy)) ok = false;
	qp_export_energy_levels_3d_box_csv(quantum, filename("box-3d-degenerate.csv").c_str(), boxLength, 2);
	if (!verifyBoxLevels(root / "box-3d-degenerate.csv", 3, boxEnergy)) ok = false;
	const double levelDelta = 2.0e12;
	qp_export_two_level_comparison_csv(quantum, filename("two-level.csv").c_str(), levelDelta, 3);
	verify("two-level.csv", "Omega_over_delta,E1_pert,E2_pert,E1_exact,E2_exact", [&](int row) {
		const double ratio = 2.0 * (row + 1) / 3.0;
		const double perturbative = hbar * levelDelta * (0.5 + ratio * ratio / 4.0);
		const double exact = hbar * levelDelta * std::sqrt(1.0 + ratio * ratio) / 2.0;
		return std::vector<double>{ratio, perturbative, -perturbative, exact, -exact};
	});
	qp_export_dos_free_csv(quantum, filename("dos-free.csv").c_str(), 3.0e-19, 3);
	verify("dos-free.csv", "E,g1D,g2D,g3D", [&](int i) {
		const double energy = (i + 1) * 1.0e-19;
		return std::vector<double>{energy, std::sqrt(2.0 * mass / (hbar * hbar * energy)) / pi,
			mass / (pi * hbar * hbar), std::pow(2.0 * mass / (hbar * hbar), 1.5) * std::sqrt(energy) / (2.0 * pi * pi)};
	});
	const double wellLength = 1.0e-9;
	const double wellGround = hbar * hbar * pi * pi / (2.0 * mass * wellLength * wellLength);
	qp_export_dos_quantum_well_csv(quantum, filename("dos-well.csv").c_str(), wellLength, 6.75 * wellGround, 2, 3);
	verify("dos-well.csv", "E,g_2DEG", [&](int i) {
		return std::vector<double>{2.25 * (i + 1) * wellGround, (i == 0 ? 1.0 : 2.0) * mass / (pi * hbar * hbar)};
	});
	const double charge = 1.602176634e-19;
	qp_export_landau_spectrum_csv(quantum, filename("landau.csv").c_str(), mass, 3.0, 0, 3);
	verify("landau.csv", "B,n,E", [&](int i) {
		const double field = i + 1.0;
		return std::vector<double>{field, 0.0, hbar * charge * field / (2.0 * mass)};
	});
	const double planck = 6.62607015e-34;
	const double landauGround = hbar * charge / (2.0 * mass);
	const double broadening = landauGround / 2.0;
	qp_export_landau_dos_csv(quantum, filename("landau-dos.csv").c_str(), 1.0, mass,
		3.0 * landauGround, 1, broadening, 3);
	verify("landau-dos.csv", "E,DOS", [&](int i) {
		const double energy = (i + 1) * landauGround;
		const double d0 = energy - landauGround;
		const double d1 = energy - 3.0 * landauGround;
		const double dos = charge / planck * broadening / pi *
			(1.0 / (d0 * d0 + broadening * broadening) +
			 1.0 / (d1 * d1 + broadening * broadening));
		return std::vector<double>{energy, dos};
	});
	qp_export_adiabatic_sweep_csv(quantum, filename("adiabatic.csv").c_str(), 1.0e-19, 3.0e-5, 3);
	verify("adiabatic.csv", "rate,Q,is_adiabatic", [&](int i) {
		const double rate = (i + 1) * 1.0e-5;
		const double q = hbar * rate / 1.0e-38;
		return std::vector<double>{rate, q, q < 0.1 ? 1.0 : 0.0};
	});
	qp_export_classical_paths_csv(quantum, filename("paths.csv").c_str(), 1.0, 3.0, 2.0, pi / 4.0, 2);
	verify("paths.csv", "tau,x_free,x_ho", [&](int i) {
		const double tau = static_cast<double>(i);
		const double harmonic = std::sin((2.0 - tau) * pi / 4.0) + 3.0 * std::sin(tau * pi / 4.0);
		return std::vector<double>{tau, 1.0 + tau, harmonic};
	});
	qp_export_free_electron_model_csv(quantum, filename("free-electron.csv").c_str(), 2);
	verify("free-electron.csv", "n,energy_J,energy_eV,occupancy", [&](int i) {
		const double n = i + 1.0;
		const double energy = hbar * hbar * pi * pi * n * n / (2.0 * mass * 1.0e-20);
		return std::vector<double>{n, energy, energy / charge, i == 0 ? 2.0 : 0.0};
	});
	qp_export_helium_variational_csv(quantum, filename("helium.csv").c_str(), 2);
	verify("helium.csv", "lambda,energy_J,energy_eV", [&](int i) {
		const double lambda = 1.0 + 0.75 * i;
		const double energy = (lambda * lambda - 27.0 * lambda / 8.0) * 4.3597447222071e-18;
		return std::vector<double>{lambda, energy, energy / charge};
	});
	qp_export_quantum_channels_csv(quantum, filename("channels.csv").c_str(), 1.0, 0.0, 0.0, 2);
	verify("channels.csv", "param,purity_ampDamp,purity_phaseDamp,purity_depol,rz_ampDamp,rz_phaseDamp,rz_depol", [&](int i) {
		const double p = i / 2.0;
		return std::vector<double>{p, (2.0 - p + p * p) / 2.0, (2.0 - p) / 2.0,
			(1.0 + (1.0 - p) * (1.0 - p)) / 2.0, p, 0.0, 0.0};
	});
	qp_export_bloch_evolution_csv(quantum, filename("bloch.csv").c_str(), 0.5, -0.5, 0.0, 2.0, 2.0, 1.0, 2.0, 2);
	verify("bloch.csv", "t,rx,ry,rz,r_mag,purity", [&](int i) {
		const double decay = std::pow(0.5, i);
		const double rx = 0.5 * decay;
		const double ry = -rx;
		const double rz = 1.0 - decay;
		const double magnitudeSquared = rx * rx + ry * ry + rz * rz;
		return std::vector<double>{static_cast<double>(i), rx, ry, rz, std::sqrt(magnitudeSquared),
			(1.0 + magnitudeSquared) / 2.0};
	});
	const double transferWidth = 1.0e-10;
	const double transferHeight = 6.0e-19;
	const double transferWidths[] = {transferWidth, transferWidth};
	const double transferHeights[] = {transferHeight, transferHeight};
	const auto transferReference = [&](int i) {
		const double energy = (i + 1) * 1.0e-19;
		const double kappa = std::sqrt(2.0 * mass * (transferHeight - energy)) / hbar;
		const double s = std::sinh(2.0 * transferWidth * kappa);
		const double transmission = 1.0 / (1.0 + transferHeight * transferHeight * s * s /
			(4.0 * energy * (transferHeight - energy)));
		return std::vector<double>{energy, transmission, 1.0 - transmission};
	};
	qp_export_transfer_matrix_csv(quantum, filename("transfer.csv").c_str(), transferWidths, transferHeights, 2, 3.0e-19, 3);
	verify("transfer.csv", "E,T,R", transferReference);
	qp_export_resonant_tunneling_csv(quantum, filename("double-barrier.csv").c_str(),
		transferHeight, transferWidth, 0.0, 3.0e-19, 3);
	verify("double-barrier.csv", "E,T,R", transferReference);
	const double incidentEnergy = 1.0e-19;
	const double potential = 2.0e-20;
	const double radius = 1.0e-10;
	qp_export_born_differential_csv(quantum, filename("born.csv").c_str(), incidentEnergy, potential, radius, 2);
	verify("born.csv", "theta,q,f_born,dsigma_born", [&](int i) {
		const double theta = i * pi / 2.0;
		const double q = 2.0 * std::sqrt(2.0 * mass * incidentEnergy) / hbar * std::sin(theta / 2.0);
		const double amplitude = i == 0 ? 2.0 * mass * potential * radius * radius * radius / (3.0 * hbar * hbar)
			: 2.0 * mass * potential / (hbar * hbar * q * q * q) * (std::sin(q * radius) - q * radius * std::cos(q * radius));
		return std::vector<double>{theta, q, amplitude, amplitude * amplitude};
	});
	const double gamowMass = 1.0e-27;
	const double gamowRadius = 1.0e-14;
	const double coulombStrength = charge * charge / (4.0 * pi * 8.854187817e-12);
	const double barrier = coulombStrength / gamowRadius;
	qp_export_gamow_tunneling_csv(quantum, filename("gamow-tunneling.csv").c_str(),
		1.0, 1.0, gamowMass, gamowRadius, 1.5 * barrier, 3);
	verify("gamow-tunneling.csv", "E,T,log10_T,G", [&](int i) {
		const double energy = (i + 1) * barrier / 2.0;
		if (i >= 1) return std::vector<double>{energy, 1.0, 0.0, 0.0};
		const double turningPoint = coulombStrength / energy;
		const double action = std::sqrt(2.0 * gamowMass * energy) * turningPoint / hbar *
			(pi / 4.0 - 0.5);
		return std::vector<double>{energy, std::exp(-2.0 * action), -2.0 * action / std::log(10.0), action};
	});
	const double scatteringK = std::sqrt(2.0 * mass * incidentEnergy) / hbar;
	const double scatteringRadius = 0.7 / scatteringK;
	const double scatteringDepth = 3.0 * incidentEnergy;
	const auto sWave = [&](double energy) {
		const double k = std::sqrt(2.0 * mass * energy) / hbar;
		const double K = std::sqrt(2.0 * mass * (energy + scatteringDepth)) / hbar;
		const double phase = std::atan2(k * std::sin(K * scatteringRadius), K * std::cos(K * scatteringRadius)) - k * scatteringRadius;
		return std::array<double, 3>{k, phase, 4.0 * pi * std::pow(std::sin(phase) / k, 2.0)};
	};
	qp_export_partial_wave_csv(quantum, filename("partial-wave.csv").c_str(), scatteringDepth, scatteringRadius, 0, incidentEnergy, 3);
	verify("partial-wave.csv", "E,k,sigma_total,delta_0,sigma_0", [&](int i) {
		const double energy = (i + 1) * incidentEnergy / 3.0;
		const auto result = sWave(energy);
		return std::vector<double>{energy, result[0], result[2], result[1], result[2]};
	});
	qp_export_differential_csv(quantum, filename("differential.csv").c_str(), incidentEnergy, scatteringDepth, scatteringRadius, 0, 2);
	verify("differential.csv", "theta,dsigma_dOmega", [&](int i) {
		return std::vector<double>{i * pi / 2.0, sWave(incidentEnergy)[2] / (4.0 * pi)};
	});
	const double freeTime = 1.0e-15;
	qp_export_free_particle_propagator_csv(quantum, filename("free-propagator.csv").c_str(), 0.0, freeTime, 3);
	verify("free-propagator.csv", "xb,K_mod2,Re_K,Im_K", [&](int i) {
		const double x = (i - 1) * 5.0e-9;
		const double magnitudeSquared = mass / (2.0 * pi * hbar * freeTime);
		const double phase = mass * x * x / (2.0 * hbar * freeTime) - pi / 4.0;
		return std::vector<double>{x, magnitudeSquared, std::sqrt(magnitudeSquared) * std::cos(phase), std::sqrt(magnitudeSquared) * std::sin(phase)};
	});
	const double omega = 1.0e12;
	qp_export_ho_propagator_csv(quantum, filename("ho-propagator.csv").c_str(), 0.0, pi / (2.0 * omega), omega, 3);
	verify("ho-propagator.csv", "xb,K_mod2,Re_K,Im_K", [&](int i) {
		const double x = (i - 1) * 5.0e-9;
		const double magnitudeSquared = mass * omega / (2.0 * pi * hbar);
		const double component = std::sqrt(magnitudeSquared / 2.0);
		return std::vector<double>{x, magnitudeSquared, component, -component};
	});
	qp_export_quantum_well_subbands_csv(quantum, filename("subbands.csv").c_str(), wellLength, mass, 1, 3);
	verify("subbands.csv", "k_parallel,n,E", [&](int i) {
		const double k = i * 2.5e9;
		return std::vector<double>{k, 1.0, wellGround + hbar * hbar * k * k / (2.0 * mass)};
	});
	qp_export_effective_potential_csv(quantum, filename("effective-potential.csv").c_str(), 2, 3);
	verify("effective-potential.csv", "r,V_eff,V_centrifugal", [&](int i) {
		const double r = (i + 1) * 5.0e-10 / 3.0;
		const double centrifugal = 3.0 * hbar * hbar / (mass * r * r);
		return std::vector<double>{r, centrifugal, centrifugal};
	});
	qp_export_ladder_operator_action_csv(quantum, filename("orbital-ladder.csv").c_str(), 1);
	verify("orbital-ladder.csv", "l,m,Lplus_coeff,Lplus_target_m,Lminus_coeff,Lminus_target_m", [&](int i) {
		const int m = i - 1;
		return std::vector<double>{1.0, static_cast<double>(m), m == 1 ? 0.0 : std::sqrt(2.0),
			static_cast<double>(m == 1 ? m : m + 1), m == -1 ? 0.0 : std::sqrt(2.0),
			static_cast<double>(m == -1 ? m : m - 1)};
	});
	qp_export_orbital_angular_momentum_csv(quantum, filename("orbital.csv").c_str(), 2);
	verify("orbital.csv", "l,m,L2_over_hbar2,Lz_over_hbar,degeneracy", [&](int i) {
		const int l = i == 0 ? 0 : (i < 4 ? 1 : 2);
		const int m = i - l * l - l;
		return std::vector<double>{static_cast<double>(l), static_cast<double>(m),
			static_cast<double>(l * (l + 1)), static_cast<double>(m), static_cast<double>(2 * l + 1)};
	}, 9);
	qp_export_orbital_angular_momentum_csv(quantum, filename("orbital-empty.csv").c_str(), -1);
	verify("orbital-empty.csv", "l,m,L2_over_hbar2,Lz_over_hbar,degeneracy", [](int) {
		return std::vector<double>{};
	}, 0);
	qp_export_spherical_harmonics_csv(quantum, filename("spherical-harmonics.csv").c_str(), 0, 3);
	verify("spherical-harmonics.csv", "theta,phi,m,ReY,ImY,absY2", [&](int i) {
		return std::vector<double>{(i / 3) * pi / 2.0, (i % 3) * pi, 0.0,
			1.0 / std::sqrt(4.0 * pi), 0.0, 1.0 / (4.0 * pi)};
	}, 9);
	qp_export_spherical_well_energy_levels_csv(quantum, filename("spherical-levels.csv").c_str(), 1.0e-10, 3, 0);
	verify("spherical-levels.csv", "n,l,g_nl,E", [&](int i) {
		const double n = i + 1.0;
		const double zero = n * pi;
		return std::vector<double>{n, 0.0, zero, hbar * hbar * zero * zero / (2.0 * mass * 1.0e-20)};
	});
	const double invSqrt2 = 1.0 / std::sqrt(2.0);
	qp_export_coupled_states_csv(quantum, filename("coupled.csv").c_str(), 0.5, 0.5);
	const std::array<std::array<double, 7>, 6> coupled = {{
		{{0.5, -0.5, 0.5, 0.5, 0, 0, -invSqrt2}},
		{{0.5, 0.5, 0.5, -0.5, 0, 0, invSqrt2}},
		{{0.5, -0.5, 0.5, -0.5, 1, -1, 1}},
		{{0.5, -0.5, 0.5, 0.5, 1, 0, invSqrt2}},
		{{0.5, 0.5, 0.5, -0.5, 1, 0, invSqrt2}},
		{{0.5, 0.5, 0.5, 0.5, 1, 1, 1}}
	}};
	verify("coupled.csv", "j1,m1,j2,m2,J,M,CG", [&](int i) {
		return std::vector<double>(coupled[i].begin(), coupled[i].end());
	}, 6);
	qp_export_singlet_triplet_csv(quantum, filename("singlet-triplet.csv").c_str());
	if (!verifyLabeledCsv(root / "singlet-triplet.csv", "state_name,S,M_S,c_upup,c_updown,c_downup,c_downdown", {
		{"triplet_+1", {1, 1, 1, 0, 0, 0}},
		{"triplet_0", {1, 0, 0, invSqrt2, invSqrt2, 0}},
		{"triplet_-1", {1, -1, 0, 0, 0, 1}},
		{"singlet", {0, 0, 0, invSqrt2, -invSqrt2, 0}}
	})) ok = false;
	qp_export_bell_state_analysis_csv(quantum, filename("bell-analysis.csv").c_str());
	if (!verifyLabeledCsv(root / "bell-analysis.csv", "state,concurrence,entropy_A,entropy_B,rhoA_00,rhoA_11", {
		{"Phi+", {1.0, std::log(2.0), std::log(2.0), 0.5, 0.5}},
		{"Phi-", {1.0, std::log(2.0), std::log(2.0), 0.5, 0.5}},
		{"Psi+", {1.0, std::log(2.0), std::log(2.0), 0.5, 0.5}},
		{"Psi-", {1.0, std::log(2.0), std::log(2.0), 0.5, 0.5}}
	})) ok = false;
	const double complexBell[] = {invSqrt2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, invSqrt2};
	qp_export_two_qubit_state_csv(quantum, filename("two-qubit.csv").c_str(), complexBell);
	if (!verifyTwoQubitCsv(root / "two-qubit.csv")) {
		std::cerr << "FAIL: invalid two-qubit CSV\n";
		ok = false;
	}
	const double singlet[] = {0.0, 0.0, invSqrt2, 0.0, -invSqrt2, 0.0, 0.0, 0.0};
	qp_export_chsh_sweep_csv(quantum, filename("chsh.csv").c_str(), singlet, 3);
	verify("chsh.csv", "thetaB,E_correlator,S_chsh", [&](int i) {
		const double theta = i * pi / 3.0;
		return std::vector<double>{theta, -std::cos(theta), -2.0 * std::cos(theta) - std::sqrt(2.0) * std::sin(theta)};
	}, 4);
	qp_export_spin_analysis_csv(quantum, filename("spin-analysis.csv").c_str(), invSqrt2, 0.0, 0.0, invSqrt2);
	if (!verifyLabeledCsv(root / "spin-analysis.csv", "quantity,value", {
		{"P_up", {0.5}}, {"P_down", {0.5}}, {"Sx", {0.0}}, {"Sy", {hbar / 2.0}}, {"Sz", {0.0}}
	})) ok = false;
	const std::vector<std::pair<std::string, std::array<double, 8>>> pauliMatrices = {
		{"sigma_x", {0, 0, 1, 0, 1, 0, 0, 0}},
		{"sigma_y", {0, 0, 0, -1, 0, 1, 0, 0}},
		{"sigma_z", {1, 0, 0, 0, 0, 0, -1, 0}},
		{"S_plus", {0, 0, hbar, 0, 0, 0, 0, 0}},
		{"S_minus", {0, 0, 0, 0, hbar, 0, 0, 0}}
	};
	std::vector<std::pair<std::string, std::vector<double>>> matrixRows;
	for (const auto& matrix : pauliMatrices) {
		for (int element = 0; element < 4; ++element) {
			matrixRows.push_back({matrix.first, {static_cast<double>(element / 2), static_cast<double>(element % 2),
				matrix.second[2 * element], matrix.second[2 * element + 1]}});
		}
	}
	qp_export_pauli_matrices_csv(quantum, filename("pauli.csv").c_str());
	if (!verifyLabeledCsv(root / "pauli.csv", "matrix,row,col,Re,Im", matrixRows)) ok = false;
	const std::vector<std::pair<std::string, std::array<double, 8>>> gates = {
		{"I", {1, 0, 0, 0, 0, 0, 1, 0}},
		{"X", {0, 0, 1, 0, 1, 0, 0, 0}},
		{"Y", {0, 0, 0, -1, 0, 1, 0, 0}},
		{"Z", {1, 0, 0, 0, 0, 0, -1, 0}},
		{"H", {invSqrt2, 0, invSqrt2, 0, invSqrt2, 0, -invSqrt2, 0}},
		{"S", {1, 0, 0, 0, 0, 0, 0, 1}},
		{"T", {1, 0, 0, 0, 0, 0, invSqrt2, invSqrt2}},
		{"Rx_pi2", {invSqrt2, 0, 0, -invSqrt2, 0, -invSqrt2, invSqrt2, 0}},
		{"Ry_pi2", {invSqrt2, 0, -invSqrt2, 0, invSqrt2, 0, invSqrt2, 0}},
		{"Rz_pi2", {invSqrt2, -invSqrt2, 0, 0, 0, 0, invSqrt2, invSqrt2}}
	};
	matrixRows.clear();
	for (const auto& gate : gates) {
		for (int element = 0; element < 4; ++element) {
			matrixRows.push_back({gate.first, {static_cast<double>(element / 2), static_cast<double>(element % 2),
				gate.second[2 * element], gate.second[2 * element + 1]}});
		}
	}
	qp_export_gate_matrices_csv(quantum, filename("gates.csv").c_str());
	if (!verifyLabeledCsv(root / "gates.csv", "gate,row,col,Re,Im", matrixRows)) ok = false;
	qp_export_ho_ladder_matrix_csv(quantum, filename("ho-ladder.csv").c_str(), 3);
	if (!verifyLabeledCsv(root / "ho-ladder.csv", "operator,row,col,value", {
		{"a", {0, 1, 1}}, {"a_dag", {1, 0, 1}}, {"N", {0, 0, 0}},
		{"a", {1, 2, std::sqrt(2.0)}}, {"a_dag", {2, 1, std::sqrt(2.0)}},
		{"N", {1, 1, 1}}, {"N", {2, 2, 2}}
	})) ok = false;
	const double oscillatorOmega = 1.0e14;
	const double positionScale = std::sqrt(hbar / (mass * oscillatorOmega));
	const double momentumScale = std::sqrt(mass * hbar * oscillatorOmega);
	qp_export_coherent_state_csv(quantum, filename("coherent-state.csv").c_str(), 1.0, oscillatorOmega, 2, 2);
	if (!verifyCoherentStateCsv(root / "coherent-state.csv", mass, oscillatorOmega, hbar, pi)) ok = false;
	qp_export_wigner_csv(quantum, filename("wigner.csv").c_str(), 0.25, -0.5, oscillatorOmega, 2, 2);
	verify("wigner.csv", "x,p,W", [&](int i) {
		const double x = (i / 2 - 1) * 5.0 * positionScale;
		const double p = (i % 2 - 1) * 5.0 * momentumScale;
		const double dx = x / positionScale - std::sqrt(2.0) * 0.25;
		const double dp = p / momentumScale + std::sqrt(2.0) * 0.5;
		return std::vector<double>{x, p, std::exp(-dx * dx - dp * dp) / (pi * hbar)};
	}, 4);
	qp_export_ho_wavefunctions_csv(quantum, filename("ho-wavefunctions.csv").c_str(), 1, oscillatorOmega, 3);
	verify("ho-wavefunctions.csv", "x,psi0,psi1", [&](int i) {
		const double x = -1.0e-9 + i * 2.0e-9 / 3.0;
		const double alpha = mass * oscillatorOmega / hbar;
		const double ground = std::pow(alpha / pi, 0.25) * std::exp(-alpha * x * x / 2.0);
		return std::vector<double>{x, ground, std::sqrt(2.0 * alpha) * x * ground};
	});
	const double electricField = 1.0e8;
	const double iswUnit = hbar * hbar * pi * pi / (2.0 * mass * 1.0e-20);
	const double coupling12 = charge * electricField * 16.0e-10 / (9.0 * pi * pi);
	const double secondOrder = coupling12 * coupling12 / (3.0 * iswUnit);
	const double firstOrder = charge * electricField * 0.5e-10;
	qp_export_stark_isw_csv(quantum, filename("stark-isw.csv").c_str(), electricField, 2, 2);
	verify("stark-isw.csv", "n,E0,E1_correction,E2_correction,E_total", [&](int i) {
		const double n = i + 1.0;
		const double energy = n * n * iswUnit;
		const double correction = i == 0 ? -secondOrder : secondOrder;
		return std::vector<double>{n, energy, firstOrder, correction, energy + firstOrder + correction};
	}, 2);
	qp_export_stark_ho_csv(quantum, filename("stark-ho.csv").c_str(), oscillatorOmega, electricField, 2);
	verify("stark-ho.csv", "n,E0,E1_correction,E2_correction,E_total", [&](int i) {
		const double energy = hbar * oscillatorOmega * (i + 0.5);
		const double shift = -charge * charge * electricField * electricField / (2.0 * mass * oscillatorOmega * oscillatorOmega);
		return std::vector<double>{static_cast<double>(i), energy, 0.0, shift, energy + shift};
	});
	const double levelEnergy = hbar * 1.0e12;
	qp_export_degenerate_two_level_csv(quantum, filename("degenerate.csv").c_str(), levelEnergy, 1.0e12, 2);
	verify("degenerate.csv", "Omega,E_plus,E_minus,splitting", [&](int i) {
		const double coupling = i * 5.0e11;
		return std::vector<double>{coupling, levelEnergy + hbar * coupling / 2.0,
			levelEnergy - hbar * coupling / 2.0, hbar * coupling};
	});
	const double deltaStrength = 1.0e-29;
	qp_export_delta_potential_wavefunction_csv(quantum, filename("delta-wavefunction.csv").c_str(), deltaStrength, 3);
	verify("delta-wavefunction.csv", "x,psi", [&](int i) {
		const double x = -1.0e-9 + i * 2.0e-9 / 3.0;
		const double kappa = mass * deltaStrength / (hbar * hbar);
		return std::vector<double>{x, std::sqrt(kappa) * std::exp(-kappa * std::abs(x))};
	});
	qp_export_parabolic_well_wavefunction_csv(quantum, filename("parabolic-wavefunction.csv").c_str(), 1, oscillatorOmega, 3);
	verify("parabolic-wavefunction.csv", "x,psi", [&](int i) {
		const double x = -1.0e-9 + i * 2.0e-9 / 3.0;
		const double alpha = mass * oscillatorOmega / hbar;
		return std::vector<double>{x, std::sqrt(2.0 * alpha) * x * std::pow(alpha / pi, 0.25) * std::exp(-alpha * x * x / 2.0)};
	});
	const double bohrRadius = 5.29177210903e-11;
	qp_export_hydrogen_radial_csv(quantum, filename("hydrogen-1s.csv").c_str(), 1, 0, 1.0, 3);
	verify("hydrogen-1s.csv", "r,R_nl,r2_R2", [&](int i) {
		const double r = 4.0 * bohrRadius * (i + 1) / 3.0;
		const double radial = 2.0 * std::pow(bohrRadius, -1.5) * std::exp(-r / bohrRadius);
		return std::vector<double>{r, radial, r * r * radial * radial};
	});
	qp_export_ho_wavefunction_csv(quantum, filename("ho-ground.csv").c_str(), 0, oscillatorOmega, 3);
	verify("ho-ground.csv", "x,psi", [&](int i) {
		const double x = -1.0e-9 + i * 2.0e-9 / 3.0;
		const double alpha = mass * oscillatorOmega / hbar;
		return std::vector<double>{x, std::pow(alpha / pi, 0.25) * std::exp(-alpha * x * x / 2.0)};
	});
	qp_export_wkb_comparison_csv(quantum, filename("wkb-ho.csv").c_str(), oscillatorOmega, 2);
	verify("wkb-ho.csv", "n,E_exact,E_wkb,error_percent", [&](int i) {
		const double energy = hbar * oscillatorOmega * (i + 0.5);
		return std::vector<double>{static_cast<double>(i), energy, energy, 0.0};
	});
	qp_export_hydrogen_probability_csv(quantum, filename("hydrogen-probability.csv").c_str(), 1, 0, 0, 1.0, 1, 2);
	verify("hydrogen-probability.csv", "r,theta,prob_density", [&](int i) {
		const double r = 4.0 * bohrRadius;
		return std::vector<double>{r, pi * i / 2.0, std::exp(-2.0 * r / bohrRadius) / (pi * std::pow(bohrRadius, 3))};
	});
	const double reducedMass = 1.66053906660e-27 / 2.0;
	qp_export_gamow_peak_csv(quantum, filename("gamow-peak.csv").c_str(), 1.0, 1.0, reducedMass, 1.0e7, 4.0e7, 3);
	verify("gamow-peak.csv", "T_K,E0_keV,Delta_keV,EG_keV", [&](int i) {
		const double temperature = 1.0e7 + (i + 1) * 1.0e7;
		const double thermalEnergy = 1.380649e-23 * temperature;
		const double gamowEnergy = 2.0 * reducedMass * std::pow(2.99792458e8 * pi / 137.035999084, 2);
		const double peak = std::cbrt(gamowEnergy * thermalEnergy * thermalEnergy / 4.0);
		const double keV = 1.602176634e-16;
		return std::vector<double>{temperature, peak / keV, 4.0 * std::sqrt(peak * thermalEnergy / 3.0) / keV, gamowEnergy / keV};
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
