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
	const double oscillatorOmega = 1.0e14;
	qp_export_ho_wavefunctions_csv(quantum, filename("ho-wavefunctions.csv").c_str(), 1, oscillatorOmega, 3);
	verify("ho-wavefunctions.csv", "x,psi0,psi1", [&](int i) {
		const double x = -1.0e-9 + i * 2.0e-9 / 3.0;
		const double alpha = mass * oscillatorOmega / hbar;
		const double ground = std::pow(alpha / pi, 0.25) * std::exp(-alpha * x * x / 2.0);
		return std::vector<double>{x, ground, std::sqrt(2.0 * alpha) * x * ground};
	});
	const double electricField = 1.0e8;
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
