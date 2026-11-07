#include "PhysicsEngine.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <string>

namespace {

bool approx(double actual, double expected, double relativeTolerance = 1.0e-9)
{
	const double scale = std::max({std::abs(actual), std::abs(expected), 1.0e-300});
	return std::abs(actual - expected) <= relativeTolerance * scale;
}

#define CHECK(condition, message) \
	do { \
		if (!(condition)) { \
			std::cerr << "FAIL: " << message << "\n"; \
			return false; \
		} \
	} while (false)

bool test_metadata()
{
	static const char* expectedNames[] = {
		"QuantumCore",
		"ClassicalMechanicsCore",
		"ElectrodynamicsCore",
		"FluidDynamicsCore",
		"NuclearCore",
		"OpticsCore",
		"SolidStateCore",
		"ThermodynamicsCore"
	};

	CHECK(approx(static_cast<double>(pe_module_count()), 8.0), "module count");
	CHECK(std::string(pe_version()) == "1.0.0", "engine version");

	for (int i = 0; i < pe_module_count(); ++i) {
		CHECK(pe_module_name(i) != nullptr, "module name is non-null");
		CHECK(std::string(pe_module_name(i)) == expectedNames[i], "module name");
		CHECK(pe_module_sim_count(i) == 50, "module simulation count");
	}

	CHECK(pe_module_name(-1) == nullptr, "negative module index returns null");
	CHECK(pe_module_name(pe_module_count()) == nullptr, "out-of-range module index returns null");
	CHECK(pe_module_sim_count(-1) == 0, "negative simulation-count index returns zero");
	CHECK(pe_module_sim_count(pe_module_count()) == 0, "out-of-range simulation-count index returns zero");
	return true;
}

bool test_quantum_states()
{
	const double s = 1.0 / std::sqrt(2.0);
	const auto matches = [](const double* actual, const double* expected, int size) {
		for (int i = 0; i < size; ++i) {
			if (std::abs(actual[i] - expected[i]) > 1.0e-12 || !std::isfinite(actual[i])) return false;
		}
		return true;
	};
	const double identity[8] = {1, 0, 0, 0, 0, 0, 1, 0};
	const double x[8] = {0, 0, 1, 0, 1, 0, 0, 0};
	const double y[8] = {0, 0, 0, -1, 0, 1, 0, 0};
	const double z[8] = {1, 0, 0, 0, 0, 0, -1, 0};
	const double hadamard[8] = {s, 0, s, 0, s, 0, -s, 0};
	double gate[9] = {};
	gate[8] = 123.0;
	qp_gate_identity(gate);
	CHECK(matches(gate, identity, 8) && gate[8] == 123.0, "Identity gate and buffer boundary");
	qp_gate_pauli_x(gate);
	CHECK(matches(gate, x, 8), "Pauli X gate");
	qp_gate_pauli_y(gate);
	CHECK(matches(gate, y, 8), "Pauli Y gate");
	qp_gate_pauli_z(gate);
	CHECK(matches(gate, z, 8), "Pauli Z gate");
	qp_gate_hadamard(gate);
	CHECK(matches(gate, hadamard, 8), "Hadamard gate");
	const double phaseS[8] = {1, 0, 0, 0, 0, 0, 0, 1};
	qp_gate_phase_s(gate);
	CHECK(matches(gate, phaseS, 8), "Phase S gate");
	const double phaseT[8] = {1, 0, 0, 0, 0, 0, s, s};
	qp_gate_t(gate);
	CHECK(matches(gate, phaseT, 8), "T gate");
	const double pi = 3.14159265358979323846;
	const double rxPi[8] = {0, 0, 0, -1, 0, -1, 0, 0};
	const double ryPi[8] = {0, 0, -1, 0, 1, 0, 0, 0};
	const double rzPi[8] = {0, -1, 0, 0, 0, 0, 0, 1};
	qp_gate_rx(pi, gate);
	CHECK(matches(gate, rxPi, 8), "X rotation gate");
	qp_gate_ry(pi, gate);
	CHECK(matches(gate, ryPi, 8), "Y rotation gate");
	qp_gate_rz(pi, gate);
	CHECK(matches(gate, rzPi, 8), "Z rotation gate");
	double product[8] = {};
	qp_multiply_spin_matrices(x, x, product);
	CHECK(matches(product, identity, 8), "Pauli X squared");
	const double spinUp[4] = {1, 0, 0, 0};
	const double spinDown[4] = {0, 0, 1, 0};
	double spinResult[4] = {};
	qp_apply_gate_to_spinor(x, spinUp, spinResult);
	CHECK(matches(spinResult, spinDown, 4), "Apply X gate to spinor");

	double rho[8] = {};
	const double plusRho[8] = {0.5, 0, 0.5, 0, 0.5, 0, 0.5, 0};
	qp_density_matrix_2x2(s, 0, s, 0, rho);
	CHECK(matches(rho, plusRho, 8), "Pure-state density matrix");
	double mixed[8] = {};
	const double mixedExpected[8] = {0.25, 0, 0, 0, 0, 0, 0.75, 0};
	qp_mixed_state_diagonal(0.25, mixed);
	CHECK(matches(mixed, mixedExpected, 8), "Diagonal mixed state");
	double blochRho[8] = {};
	qp_density_matrix_from_bloch(1.0, 0.0, 0.0, blochRho);
	CHECK(matches(blochRho, plusRho, 8), "Density matrix from Bloch vector");
	double bx = 0.0, by = 0.0, bz = 0.0;
	qp_bloch_vector_from_rho(rho, &bx, &by, &bz);
	CHECK(std::abs(bx - 1.0) < 1.0e-12 && std::abs(by) < 1.0e-12 &&
		  std::abs(bz) < 1.0e-12, "Bloch vector from density matrix");
	qp_qubit_bloch_vector(1.0, 0.0, 0.0, 1.0, &bx, &by, &bz);
	CHECK(std::abs(bx) < 1.0e-12 && std::abs(by - 1.0) < 1.0e-12 &&
		  std::abs(bz) < 1.0e-12, "Bloch vector from complex amplitudes");
	CHECK(approx(qp_von_neumann_entropy_2x2(mixed),
			  -0.25 * std::log(0.25) - 0.75 * std::log(0.75)), "Mixed-state entropy");
	CHECK(approx(qp_fidelity_2x2(rho, rho), 1.0), "Identical-state fidelity");
	double channel[8] = {};
	const double damped[8] = {0.625, 0, s * 0.5 * std::sqrt(1.5), 0,
								 s * 0.5 * std::sqrt(1.5), 0, 0.375, 0};
	qp_amplitude_damping_channel(rho, 0.25, channel);
	CHECK(matches(channel, damped, 8), "Amplitude damping channel");
	const double dephased[8] = {0.5, 0, 0.25, 0, 0.25, 0, 0.5, 0};
	qp_phase_damping_channel(rho, 0.75, channel);
	CHECK(matches(channel, dephased, 8), "Phase damping channel");
	const double depolarized[8] = {0.5, 0, 0.375, 0, 0.375, 0, 0.5, 0};
	qp_depolarizing_channel(rho, 0.25, channel);
	CHECK(matches(channel, depolarized, 8), "Depolarizing channel");

	double phi[8] = {}, psi[8] = {};
	const double phiExpected[8] = {s, 0, 0, 0, 0, 0, s, 0};
	const double psiExpected[8] = {0, 0, s, 0, -s, 0, 0, 0};
	qp_bell_state_phi(1, phi);
	qp_bell_state_psi(0, psi);
	CHECK(matches(phi, phiExpected, 8) && matches(psi, psiExpected, 8), "Bell-state amplitudes");
	CHECK(approx(qp_concurrence(phi), 1.0), "Bell-state concurrence");
	CHECK(approx(qp_chsh_correlator(phi, 0.0, 0.0), 1.0), "Bell-state Z correlation");
	CHECK(approx(qp_measure_qubit_prob(phi, 0, 0), 0.5) &&
		  approx(qp_measure_qubit_prob(phi, 1, 1), 0.5), "Bell-state measurement probabilities");
	double rho4[32] = {}, reducedA[8] = {}, reducedB[8] = {};
	qp_density_matrix_from_state(phi, rho4);
	CHECK(std::abs(rho4[0] - 0.5) < 1.0e-12 && std::abs(rho4[6] - 0.5) < 1.0e-12 &&
		  std::abs(rho4[24] - 0.5) < 1.0e-12 && std::abs(rho4[30] - 0.5) < 1.0e-12,
		  "Bell density-matrix entries");
	qp_partial_trace_a(rho4, reducedA);
	qp_partial_trace_b(rho4, reducedB);
	const double maximallyMixed[8] = {0.5, 0, 0, 0, 0, 0, 0.5, 0};
	CHECK(matches(reducedA, maximallyMixed, 8) && matches(reducedB, maximallyMixed, 8),
		  "Bell-state partial traces");
	const double basis00[8] = {1, 0, 0, 0, 0, 0, 0, 0};
	const double basis10[8] = {0, 0, 0, 0, 1, 0, 0, 0};
	const double basis01[8] = {0, 0, 1, 0, 0, 0, 0, 0};
	const double basis11[8] = {0, 0, 0, 0, 0, 0, 1, 0};
	double state[8] = {};
	qp_apply_single_qubit_gate(basis00, x, 0, state);
	CHECK(matches(state, basis10, 8), "Apply X to first qubit");
	qp_apply_single_qubit_gate(basis00, x, 1, state);
	CHECK(matches(state, basis01, 8), "Apply X to second qubit");
	qp_apply_cnot(basis10, 0, 1, state);
	CHECK(matches(state, basis11, 8), "Controlled-NOT gate");
	qp_apply_swap(basis10, state);
	CHECK(matches(state, basis01, 8), "SWAP gate");
	qp_apply_cz(basis11, state);
	const double negative11[8] = {0, 0, 0, 0, 0, 0, -1, 0};
	CHECK(matches(state, negative11, 8), "Controlled-Z gate");
	qp_collapse_after_measurement(phi, 0, 1, state);
	CHECK(matches(state, basis11, 8), "Bell-state measurement collapse");

	double allowedJ[3] = {-1.0, -1.0, 99.0};
	CHECK(qp_list_allowed_j(1.5, 1.0, allowedJ, 2) == 3 &&
		  approx(allowedJ[0], 0.5) && approx(allowedJ[1], 1.5) && allowedJ[2] == 99.0,
		  "Allowed J total count and truncated buffer");
	double allowedF[3] = {-1.0, -1.0, 99.0};
	CHECK(qp_list_allowed_f(1.5, 1.0, allowedF, 2) == 3 &&
		  approx(allowedF[0], 0.5) && approx(allowedF[1], 1.5) && allowedF[2] == 99.0,
		  "Allowed F total count and truncated buffer");
	CHECK(qp_list_allowed_j(1.5, 1.0, nullptr, 0) == 3, "Allowed J count query");
	return true;
}

bool test_module_apis()
{
	QP_Handle quantum = qp_create("integration-quantum", 9.1093837015e-31, 1.0e-10, 1);
	CHECK(quantum != nullptr, "Quantum handle creation");
	const double quantumEnergy = qp_energy_1d_box(quantum, 3);
	CHECK(std::isfinite(quantumEnergy) && quantumEnergy > 0.0, "Quantum energy");
	CHECK(approx(qp_get_mass(quantum), 9.1093837015e-31), "Quantum mass getter");
	double wavefunction[4] = {};
	CHECK(qp_wavefunction_1d_box(quantum, 1, 4, wavefunction) == 4,
		  "Quantum wavefunction output length");
	CHECK(approx(wavefunction[0], 0.0) && wavefunction[3] > 0.0,
		  "Quantum wavefunction output values");
	double pauliX[8] = {};
	qp_pauli_x(pauliX);
	CHECK(approx(pauliX[2], 1.0) && approx(pauliX[4], 1.0),
		  "Quantum Pauli-X output matrix");
	CHECK(approx(qp_energy_harmonic_oscillator(quantum, 2, 4.0), 10.0),
		  "Quantum harmonic oscillator energy");
	CHECK(approx(qp_hermite_polynomial(3, 0.5), -5.0), "Quantum Hermite polynomial");
	double dx = 0.0;
	double dp = 0.0;
	qp_ho_uncertainty(quantum, 0, 2.0, &dx, &dp);
	CHECK(approx(dx, std::sqrt(1.0545718e-34 / (2.0 * 9.1093837015e-31 * 2.0))),
		  "Quantum position uncertainty");
	CHECK(approx(dp, std::sqrt(9.1093837015e-31 * 1.0545718e-34 * 2.0 / 2.0)),
		  "Quantum momentum uncertainty");
	const double hbar = 1.0545718e-34;
	const double deltaPotential = 1.0e-18;
	CHECK(approx(qp_delta_potential_energy(quantum, deltaPotential),
				  -9.1093837015e-31 * deltaPotential * deltaPotential /
				  (2.0 * hbar * hbar)),
		  "Quantum delta-potential energy");
	const double quantumPi = 3.14159265358979323846;
	const double quantumMass = 9.1093837015e-31;
	const double quantumCharge = 1.602176634e-19;
	const double quantumPlanck = 6.62607015e-34;
	const double quantumC = 299792458.0;
	const double quantumLength = 1.0e-9;
	const double boxEnergy = hbar * hbar * quantumPi * quantumPi /
		(2.0 * quantumMass * quantumLength * quantumLength);
	CHECK(approx(qp_get_length(quantum), 1.0e-10) && qp_get_dimension(quantum) == 1,
		  "Quantum length and dimension getters");
	CHECK(approx(qp_energy_2d_box(quantum, 1, 2, quantumLength, quantumLength), 5.0 * boxEnergy) &&
		  approx(qp_energy_3d_box(quantum, 1, 2, 3, quantumLength, quantumLength, quantumLength),
				 14.0 * boxEnergy) &&
		  approx(qp_quantum_well_energy(1, quantumLength, quantumMass, 0.0, 0.0), boxEnergy) &&
		  approx(qp_quantum_dot_energy(1, 2, 3, quantumLength, quantumLength, quantumLength, quantumMass),
				 14.0 * boxEnergy),
		  "Quantum box and confinement energies");
	CHECK(approx(qp_tight_binding_energy(5.0, 2.0, 0.0, 1.0), 1.0) &&
		  approx(qp_tight_binding_energy(5.0, 2.0, quantumPi, 1.0), 9.0) &&
		  approx(qp_tight_binding_effective_mass(2.0e-19, quantumLength),
				 hbar * hbar / (4.0e-19 * quantumLength * quantumLength)),
		  "Quantum tight-binding dispersion and effective mass");
	const double dosEnergy = 1.0e-19;
	CHECK(approx(qp_dos_1d(dosEnergy, quantumMass),
				 std::sqrt(2.0 * quantumMass / (hbar * hbar * dosEnergy)) / quantumPi) &&
		  approx(qp_dos_2d(quantumMass), quantumMass / (quantumPi * hbar * hbar)) &&
		  approx(qp_dos_3d(dosEnergy, quantumMass),
				 std::pow(2.0 * quantumMass / (hbar * hbar), 1.5) * std::sqrt(dosEnergy) /
				 (2.0 * quantumPi * quantumPi)) &&
		  approx(qp_dos_1d(0.0, quantumMass), 0.0) &&
		  approx(qp_dos_3d(-dosEnergy, quantumMass), 0.0),
		  "Quantum free-particle densities of states");
	CHECK(approx(qp_dos_quantum_well_2deg(0.5 * boxEnergy, quantumLength, quantumMass, 3), 0.0) &&
		  approx(qp_dos_quantum_well_2deg(5.0 * boxEnergy, quantumLength, quantumMass, 3),
				 2.0 * quantumMass / (quantumPi * hbar * hbar)),
		  "Quantum well density of states subband steps");
	CHECK(approx(qp_coherent_state_mean_n(2.0), 4.0) &&
		  approx(qp_coherent_state_variance_n(2.0), 4.0) &&
		  approx(qp_coherent_state_photon_prob(2.0, 0), std::exp(-4.0)) &&
		  approx(qp_coherent_state_photon_prob(2.0, 2), 8.0 * std::exp(-4.0)),
		  "Coherent-state photon statistics");
	CHECK(approx(qp_squeezed_uncertainty_x(0.5, 2.0, quantumMass),
				 std::sqrt(hbar / (4.0 * quantumMass)) * std::exp(-0.5)) &&
		  approx(qp_squeezed_uncertainty_p(0.5, 2.0, quantumMass),
				 std::sqrt(quantumMass * hbar) * std::exp(0.5)),
		  "Squeezed-state uncertainties");
	CHECK(approx(qp_berry_phase_spin_half(quantumPi / 2.0), -quantumPi) &&
		  approx(qp_berry_phase_two_level(quantumPi), -2.0 * quantumPi) &&
		  approx(qp_solid_angle_cone(quantumPi / 2.0), 2.0 * quantumPi) &&
		  approx(qp_dynamic_phase(hbar, 2.0), -2.0) &&
		  approx(qp_adiabatic_parameter(1.0e-19, 1.0e-5), hbar * 1.0e-5 / 1.0e-38),
		  "Geometric, dynamic and adiabatic phases");
	CHECK(approx(qp_landau_zener_probability(1.0e-20, 1.0e-5),
				 std::exp(-2.0 * quantumPi * 1.0e-40 / (hbar * 1.0e-5))) &&
		  approx(qp_landau_zener_probability(1.0e-20, 0.0), 0.0),
		  "Landau-Zener transition probability");
	CHECK(approx(qp_flux_quantum(), quantumPlanck / quantumCharge) &&
		  approx(qp_aharonov_bohm_phase(1.0e-15, quantumCharge), quantumCharge * 1.0e-15 / hbar) &&
		  approx(qp_ab_phase_from_flux_ratio(0.5), quantumPi),
		  "Aharonov-Bohm phase and flux quantum");
	const double cyclotron = quantumCharge * 2.0 / quantumMass;
	CHECK(approx(qp_cyclotron_frequency(2.0, quantumMass), cyclotron) &&
		  approx(qp_landau_level_energy(2, 2.0, quantumMass), 2.5 * hbar * cyclotron) &&
		  approx(qp_magnetic_length(2.0), std::sqrt(hbar / (quantumCharge * 2.0))) &&
		  approx(qp_filling_factor(1.0e15, 2.0), 1.0e15 * quantumPlanck / (quantumCharge * 2.0)),
		  "Landau levels and magnetic length");
	CHECK(approx(qp_hall_conductivity_iqhe(2), 2.0 * quantumCharge * quantumCharge / quantumPlanck) &&
		  approx(qp_hall_resistance_iqhe(2), quantumPlanck / (2.0 * quantumCharge * quantumCharge)) &&
		  approx(qp_hyperfine_energy(4.0, 1.0, 0.5, 0.5), 1.0) &&
		  approx(qp_hydrogen_21cm_frequency(), 1420.405751768e6) &&
		  approx(qp_hydrogen_21cm_wavelength(), quantumC / 1420.405751768e6),
		  "Integer quantum Hall and hyperfine quantities");
	const double restEnergy = quantumMass * quantumC * quantumC;
	const double momentum = quantumMass * quantumC;
	const double waveNumber = momentum / hbar;
	CHECK(approx(qp_relativistic_energy(0.0, quantumMass), restEnergy) &&
		  approx(qp_relativistic_energy(momentum, quantumMass), std::sqrt(2.0) * restEnergy) &&
		  approx(qp_relativistic_kinetic_energy(momentum, quantumMass), (std::sqrt(2.0) - 1.0) * restEnergy) &&
		  approx(qp_compton_wavelength(quantumMass), quantumPlanck / (quantumMass * quantumC)) &&
		  approx(qp_reduced_compton_wavelength(quantumMass), hbar / (quantumMass * quantumC)),
		  "Relativistic energy and Compton wavelengths");
	CHECK(approx(qp_klein_gordon_dispersion(waveNumber, quantumMass),
				 std::sqrt(2.0) * quantumC * waveNumber) &&
		  approx(qp_klein_gordon_group_velocity(waveNumber, quantumMass), quantumC / std::sqrt(2.0)) &&
		  approx(qp_klein_gordon_phase_velocity(waveNumber, quantumMass), quantumC * std::sqrt(2.0)),
		  "Klein-Gordon dispersion and velocities");
	qp_destroy(quantum);

	CP_Handle classical = cp_create("integration-classical", 2.0, 1.0, 3);
	CHECK(classical != nullptr, "Classical handle creation");
	CHECK(approx(cp_getMass(classical), 2.0), "Classical mass");
	CHECK(approx(cp_projectileX(classical, 20.0, 0.5, 2.0), 20.0 * std::cos(0.5) * 2.0),
		  "Classical projectile position");
	CHECK(approx(cp_freeParticlePosition(classical, 1.0, 3.0, 2.0), 7.0),
		  "Classical free-particle position");
	CHECK(approx(cp_freeParticleVelocity(classical, 3.0), 3.0),
		  "Classical free-particle velocity");
	CHECK(approx(cp_freeParticleKineticEnergy(classical, 3.0), 0.5 * 2.0 * 9.0),
		  "Classical free-particle kinetic energy");
	CHECK(approx(cp_freeParticleMomentum(classical, 3.0), 6.0),
		  "Classical free-particle momentum");
	CHECK(approx(cp_projectileY(classical, 20.0, 0.5, 2.0, 9.81),
				  20.0 * std::sin(0.5) * 2.0 - 0.5 * 9.81 * 4.0),
		  "Classical projectile height");
	CHECK(approx(cp_projectileRange(classical, 20.0, 0.5, 9.81),
				  400.0 * std::sin(1.0) / 9.81),
		  "Classical projectile range");
	CHECK(approx(cp_projectileMaxHeight(classical, 20.0, 0.5, 9.81),
				  std::pow(20.0 * std::sin(0.5), 2.0) / (2.0 * 9.81)),
		  "Classical projectile maximum height");
	CHECK(approx(cp_projectileTimeOfFlight(classical, 20.0, 0.5, 9.81),
				  40.0 * std::sin(0.5) / 9.81),
		  "Classical projectile time of flight");
	CHECK(approx(cp_shoPosition(classical, 2.0, 3.0, 0.0, 0.25),
				  2.0 * std::cos(0.75)),
		  "Classical oscillator position");
	CHECK(approx(cp_shoVelocity(classical, 2.0, 3.0, 0.0, 0.25),
				  -6.0 * std::sin(0.75)),
		  "Classical oscillator velocity");
	CHECK(approx(cp_shoEnergy(classical, 2.0, 3.0), 0.5 * 2.0 * 9.0 * 4.0),
		  "Classical oscillator energy");
	CHECK(approx(cp_shoPeriod(classical, 3.0), 2.0 * 3.14159265358979323846 / 3.0),
		  "Classical oscillator period");
	CHECK(approx(cp_shoFrequencyFromSpring(classical, 8.0), 2.0),
		  "Classical spring frequency");
	CHECK(approx(cp_dampedHOPosition(classical, 2.0, 3.0, 1.0, 0.5),
			  2.0 * std::exp(-0.5) * std::cos(std::sqrt(8.0) * 0.5)),
		  "Underdamped oscillator position");
	CHECK(approx(cp_dampedHOPosition(classical, 2.0, 3.0, 3.0, 0.5),
			  5.0 * std::exp(-1.5)),
		  "Critically damped oscillator position");
	CHECK(approx(cp_dampedHOEnvelope(classical, 2.0, 1.0, 0.5), 2.0 * std::exp(-0.5)),
		  "Damped oscillator envelope");
	CHECK(approx(cp_dampedHOQFactor(classical, 3.0, 0.5), 3.0),
		  "Damped oscillator quality factor");
	CHECK(cp_dampedHORegime(classical, 3.0, 1.0) == 0 &&
		  cp_dampedHORegime(classical, 3.0, 3.0) == 1 &&
		  cp_dampedHORegime(classical, 3.0, 4.0) == 2,
		  "Damped oscillator regimes");
	CHECK(approx(cp_drivenHOAmplitude(classical, 4.0, 3.0, 2.0, 0.5),
			  2.0 / std::sqrt(29.0)),
		  "Driven oscillator amplitude");
	CHECK(approx(cp_drivenHOPhase(classical, 3.0, 2.0, 0.5), std::atan2(2.0, 5.0)),
		  "Driven oscillator phase");
	CHECK(approx(cp_drivenHOResonanceFreq(classical, 3.0, 0.5), std::sqrt(8.5)) &&
		  cp_drivenHOResonanceFreq(classical, 1.0, 1.0) == 0.0,
		  "Driven oscillator resonance frequency");
	CHECK(approx(cp_drivenHOPowerAbsorbed(classical, 4.0, 3.0, 2.0, 0.5), 8.0 / 29.0),
		  "Driven oscillator absorbed power");
	CHECK(approx(cp_keplerEnergy(classical, 2.0, 4.0, 2.0, 1.0, 8.0), -2.0),
		  "Kepler energy");
	CHECK(approx(cp_keplerEffectivePotential(classical, 4.0, 2.0, 8.0, 2.0), -3.0),
		  "Kepler effective potential");
	CHECK(approx(cp_keplerOrbitRadius(classical, 4.0, 2.0, 8.0, 0.5, 0.0), 2.0 / 3.0),
		  "Kepler orbit radius");
	CHECK(approx(cp_keplerEccentricity(classical, -1.0, 4.0, 2.0, 8.0), std::sqrt(0.75)),
		  "Kepler eccentricity");
	CHECK(approx(cp_keplerPeriod(classical, 1.0, 2.0, 8.0), 3.14159265358979323846),
		  "Kepler period");
	CHECK(approx(cp_keplerSemiMajorAxis(classical, -2.0, 2.0, 8.0), 2.0),
		  "Kepler semi-major axis");
	CHECK(approx(cp_twoBodyReducedMass(classical, 2.0, 6.0), 1.5), "Two-body reduced mass");
	CHECK(approx(cp_gravitationalPE(classical, 2.0, 6.0, 3.0), -4.0 * 6.67430e-11),
		  "Gravitational potential energy");
	CHECK(approx(cp_escapeVelocity(classical, 3.0e24, 6.0e6),
			  std::sqrt(2.0 * 6.67430e-11 * 3.0e24 / 6.0e6)), "Escape velocity");
	CHECK(approx(cp_orbitalVelocity(classical, 3.0e24, 6.0e6),
			  std::sqrt(6.67430e-11 * 3.0e24 / 6.0e6)), "Orbital velocity");
	CHECK(approx(cp_hillSphereRadius(classical, 100.0, 3.0, 1000.0), 10.0),
		  "Hill sphere radius");
	CHECK(approx(cp_cyclotronFrequency(classical, -2.0, 3.0), 3.0),
		  "Cyclotron frequency with negative charge");
	CHECK(approx(cp_cyclotronRadius(classical, 12.0, -2.0, 3.0), 4.0),
		  "Cyclotron radius with negative charge");
	cp_destroy(classical);

	CHECK(approx(cp_momentOfInertiaSphere(3.0, 2.0), 4.8), "Solid sphere inertia");
	CHECK(approx(cp_momentOfInertiaCylinder(3.0, 2.0), 6.0), "Cylinder inertia");
	CHECK(approx(cp_momentOfInertiaRod(3.0, 2.0), 1.0), "Rod inertia");
	CHECK(approx(cp_momentOfInertiaDisk(3.0, 2.0), 6.0), "Disk inertia");
	CHECK(approx(cp_momentOfInertiaHollowSphere(3.0, 2.0), 8.0), "Hollow sphere inertia");
	CHECK(approx(cp_momentOfInertiaRectPlate(3.0, 2.0, 4.0), 5.0), "Plate inertia");
	CHECK(approx(cp_parallelAxisTheorem(2.0, 3.0, 2.0), 14.0), "Parallel axis theorem");
	const double lightSpeed = 299792458.0;
	const double speed = 0.6 * lightSpeed;
	CHECK(approx(cp_lorentzGamma(speed), 1.25) && approx(cp_lorentzGamma(0.0), 1.0),
		  "Lorentz factor");
	CHECK(approx(cp_relativisticMomentum(2.0, speed), 1.25 * 2.0 * speed),
		  "Relativistic momentum");
	CHECK(approx(cp_relativisticKineticEnergy(2.0, speed), 0.25 * 2.0 * lightSpeed * lightSpeed),
		  "Relativistic kinetic energy");
	CHECK(approx(cp_relativisticTotalEnergy(2.0, speed), 1.25 * 2.0 * lightSpeed * lightSpeed),
		  "Relativistic total energy");
	CHECK(approx(cp_lengthContraction(10.0, speed), 8.0), "Relativistic length contraction");
	CHECK(approx(cp_timeDilation(4.0, speed), 5.0), "Relativistic time dilation");
	CHECK(approx(cp_velocityAddition(speed, speed), 1.2 * lightSpeed / 1.36),
		  "Relativistic velocity addition");
	CHECK(approx(cp_relativisticDoppler(100.0, speed), 200.0), "Relativistic Doppler shift");
	CHECK(approx(cp_youngModulusStress(200.0, 0.01), 2.0), "Elastic stress");
	CHECK(approx(cp_poissonRatio(-0.002, 0.01), 0.2) && cp_poissonRatio(1.0, 0.0) == 0.0,
		  "Poisson ratio and zero axial strain");
	CHECK(approx(cp_shearModulus(200.0, 0.25), 80.0), "Shear modulus");
	CHECK(approx(cp_bulkModulus(150.0, 0.25), 100.0), "Bulk modulus");
	CHECK(approx(cp_waveSpeedLongitudinal(200.0, 2.0), 10.0), "Longitudinal wave speed");
	CHECK(approx(cp_waveSpeedTransverse(50.0, 2.0), 5.0), "Transverse wave speed");

	EM_Handle electromagnetic = em_create("integration-em", 2.5, 1.0, 5.8e7);
	CHECK(electromagnetic != nullptr, "Electrodynamics handle creation");
	CHECK(approx(em_get_epsilon_r(electromagnetic), 2.5), "Relative permittivity");
	CHECK(approx(em_coulomb_force(electromagnetic, 1.0e-6, 2.0e-6, 0.1),
				  8.9875517923e9 * 1.0e-6 * 2.0e-6 / (0.1 * 0.1)),
		  "Coulomb force");
	CHECK(approx(em_coulomb_potential(electromagnetic, 2.0e-6, 0.1),
				  8.9875517873681764e9 * 2.0e-6 / 0.1),
		  "Coulomb potential");
	CHECK(approx(em_dipole_torque(electromagnetic, 2.0, 3.0, 0.5),
				  2.0 * 3.0 * std::sin(0.5)),
		  "Electric dipole torque");
	CHECK(approx(em_gauss_flux(electromagnetic, 1.0e-9),
				  1.0e-9 / (2.5 * 8.854187817e-12)),
		  "Gauss-law flux");
	CHECK(approx(em_capacitor_energy(electromagnetic, 2.0e-6, 12.0),
				  0.5 * 2.0e-6 * 12.0 * 12.0),
		  "Capacitor energy");
	CHECK(approx(em_capacitance_parallel_plate(electromagnetic, 0.02, 1.0e-3),
				  2.5 * 8.854187817e-12 * 0.02 / 1.0e-3),
		  "Parallel-plate capacitance");
	CHECK(approx(em_ohm_law(electromagnetic, 12.0, 4.0), 3.0), "Ohm law current");
	CHECK(approx(em_biot_savart_wire(electromagnetic, 4.0, 0.20),
				  1.2566370614e-6 * 4.0 / (2.0 * 3.14159265358979323846 * 0.20)),
		  "Biot-Savart wire field");
	CHECK(approx(em_ampere_wire_field(electromagnetic, 4.0, 0.20),
				  1.2566370614e-6 * 4.0 / (2.0 * 3.14159265358979323846 * 0.20)),
		  "Ampere wire field");
	CHECK(approx(em_radiation_pressure(electromagnetic, 600.0, 1), 600.0 / 299792458.0),
		  "Absorbed radiation pressure");
	CHECK(approx(em_radiation_pressure(electromagnetic, 600.0, 0), 1200.0 / 299792458.0),
		  "Reflected radiation pressure");
	CHECK(approx(em_skin_depth(electromagnetic, 1.0e9),
				  std::sqrt(2.0 / (1.2566370614e-6 * 1.0 * 5.8e7 * 1.0e9))),
		  "Electromagnetic skin depth");
	CHECK(approx(em_resistance(electromagnetic, 1.0e-6, 2.0, 0.5), 4.0e-6),
		  "Electromagnetic resistance");
	CHECK(approx(em_power_dissipated(electromagnetic, 3.0, 4.0), 36.0),
		  "Electromagnetic dissipated power");
	CHECK(approx(em_rc_time_constant(electromagnetic, 1000.0, 2.0e-6), 2.0e-3),
		  "RC time constant");
	CHECK(approx(em_magnetic_dipole_torque(electromagnetic, 2.0, 3.0, 0.5),
				  6.0 * std::sin(0.5)),
		  "Magnetic dipole torque");
	CHECK(approx(em_magnetic_dipole_energy(electromagnetic, 2.0, 3.0, 0.5),
				  -6.0 * std::cos(0.5)),
		  "Magnetic dipole energy");
	CHECK(approx(em_faraday_emf(electromagnetic, 4.0), -4.0), "Faraday EMF");
	CHECK(approx(em_motional_emf(electromagnetic, 2.0, 3.0, 4.0), 24.0),
		  "Motional EMF");
	CHECK(approx(em_wave_speed_in_medium(electromagnetic),
				  299792458.0 / std::sqrt(2.5)),
		  "Wave speed in medium");
	const double emPi = 3.14159265358979323846;
	const double eps0 = 8.854187817e-12;
	const double emMu0 = 1.2566370614e-6;
	const double coulombConstant = 8.9875517873681764e9;
	const double mediumSpeed = 299792458.0 / std::sqrt(2.5);
	CHECK(approx(em_get_mu_r(electromagnetic), 1.0) &&
		  approx(em_get_sigma(electromagnetic), 5.8e7), "Electromagnetic material getters");
	CHECK(approx(em_coulomb_energy(electromagnetic, 2.0e-6, -3.0e-6, 0.5),
			  coulombConstant * 2.0e-6 * -3.0e-6 / 0.5), "Coulomb energy");
	CHECK(approx(em_dipole_potential(electromagnetic, 2.0e-6, 0.5, 0.3),
			  coulombConstant * 2.0e-6 * std::cos(0.3) / 0.25), "Electric dipole potential");
	CHECK(approx(em_dipole_energy(electromagnetic, 2.0, 3.0, 0.3), -6.0 * std::cos(0.3)),
		  "Electric dipole energy");
	CHECK(approx(em_field_infinite_plane(electromagnetic, 1.0e-9), 1.0e-9 / (2.0 * 2.5 * eps0)),
		  "Infinite charged plane field");
	CHECK(approx(em_field_infinite_line(electromagnetic, 1.0e-9, 0.2),
			  1.0e-9 / (2.0 * emPi * 2.5 * eps0 * 0.2)), "Infinite charged line field");
	const double sphereFieldSurface = 1.0e-9 / (4.0 * emPi * 2.5 * eps0 * 0.2 * 0.2);
	CHECK(approx(em_field_uniform_sphere(electromagnetic, 1.0e-9, 0.2, 0.1),
			  sphereFieldSurface * 0.5) &&
		  approx(em_field_uniform_sphere(electromagnetic, 1.0e-9, 0.2, 0.2), sphereFieldSurface) &&
		  approx(em_field_uniform_sphere(electromagnetic, 1.0e-9, 0.2, 0.4),
				 sphereFieldSurface * 0.25), "Uniform sphere field inside, surface and outside");
	CHECK(approx(em_potential_point_charge(electromagnetic, 1.0e-9, 0.5),
			  coulombConstant * 1.0e-9 / 0.5), "Point charge potential");
	CHECK(approx(em_potential_ring(electromagnetic, 1.0e-9, 0.3, 0.4),
			  coulombConstant * 1.0e-9 / 0.5), "Ring potential on axis");
	CHECK(approx(em_potential_disk(electromagnetic, 1.0e-9, 0.3, 0.4),
			  1.0e-9 / (2.0 * eps0) * 0.1), "Disk potential on axis");
	CHECK(approx(em_potential_spherical_shell(electromagnetic, 1.0e-9, 0.2, 0.1),
			  coulombConstant * 1.0e-9 / 0.2) &&
		  approx(em_potential_spherical_shell(electromagnetic, 1.0e-9, 0.2, 0.4),
				 coulombConstant * 1.0e-9 / 0.4), "Spherical shell potential inside and outside");
	CHECK(approx(em_capacitance_cylindrical(electromagnetic, 2.0, 0.1, 0.2),
			  2.0 * emPi * 2.5 * eps0 * 2.0 / std::log(2.0)), "Cylindrical capacitance");
	CHECK(approx(em_capacitance_spherical(electromagnetic, 0.1, 0.2),
			  4.0 * emPi * 2.5 * eps0 * 0.1 * 0.2 / 0.1), "Spherical capacitance");
	CHECK(approx(em_polarization_field(electromagnetic, 1.5, 100.0), eps0 * 1.5 * 100.0),
		  "Dielectric polarization");
	CHECK(approx(em_displacement_field(electromagnetic, 100.0), 2.5 * eps0 * 100.0),
		  "Electric displacement field");
	CHECK(approx(em_dielectric_capacitance(electromagnetic, 2.0e-6), 5.0e-6),
		  "Dielectric capacitance");
	CHECK(approx(em_drift_velocity(electromagnetic, 2.0, 1.0e28, 1.0e-6),
			  2.0 / (1.0e28 * 1.602176634e-19 * 1.0e-6)), "Charge carrier drift velocity");
	CHECK(em_rc_charge_voltage(electromagnetic, 12.0, 1000.0, 2.0e-6, 0.0) == 0.0 &&
		  approx(em_rc_charge_voltage(electromagnetic, 12.0, 1000.0, 2.0e-6, 2.0e-3),
				 12.0 * (1.0 - std::exp(-1.0))), "RC charging voltage");
	CHECK(approx(em_rc_discharge_current(electromagnetic, 12.0, 1000.0, 2.0e-6, 0.0), 0.012) &&
		  approx(em_rc_discharge_current(electromagnetic, 12.0, 1000.0, 2.0e-6, 2.0e-3),
				 0.012 * std::exp(-1.0)), "RC discharge current");
	CHECK(approx(em_field_center_loop(electromagnetic, 4.0, 0.2), emMu0 * 4.0 / 0.4),
		  "Circular loop center field");
	CHECK(approx(em_field_on_axis_loop(electromagnetic, 4.0, 0.2, 0.0), emMu0 * 4.0 / 0.4),
		  "Circular loop axis field at center");
	CHECK(approx(em_field_solenoid(electromagnetic, 100.0, 4.0), emMu0 * 400.0),
		  "Solenoid magnetic field");
	CHECK(approx(em_field_inside_wire(electromagnetic, 4.0, 0.2, 0.1),
			  emMu0 * 4.0 * 0.1 / (2.0 * emPi * 0.2 * 0.2)), "Field inside wire");
	CHECK(approx(em_field_toroid(electromagnetic, 100.0, 4.0, 0.2),
			  emMu0 * 400.0 / (2.0 * emPi * 0.2)), "Toroid magnetic field");
	CHECK(approx(em_langevin(electromagnetic, 1.0e-9), 1.0e-9 / 3.0), "Langevin small-field limit");
	CHECK(approx(em_brillouin(electromagnetic, 0.5, 1.0e-9), 1.0e-9), "Brillouin small-field limit");
	CHECK(approx(em_curie_weiss(electromagnetic, 4.0, 300.0, 100.0), 0.02), "Curie-Weiss susceptibility");
	CHECK(approx(em_malus_law(electromagnetic, 4.0, emPi / 3.0), 1.0), "Electromagnetic Malus law");
	CHECK(approx(em_brewster_angle(electromagnetic, 1.0, 1.5), std::atan(1.5)), "EM Brewster angle");
	CHECK(approx(em_snell_angle(electromagnetic, 1.0, 1.5, 0.4), std::asin(std::sin(0.4) / 1.5)) &&
		  em_snell_angle(electromagnetic, 1.5, 1.0, 1.2) == -1.0, "EM Snell refraction and total reflection");
	CHECK(approx(em_fresnel_rs(electromagnetic, 1.0, 1.5, 0.0), -0.2) &&
		  approx(em_fresnel_rp(electromagnetic, 1.0, 1.5, 0.0), 0.2), "Fresnel amplitude coefficients");
	CHECK(approx(em_reflectance(electromagnetic, 1.0, 1.5, 0.0, 1), 0.04) &&
		  approx(em_reflectance(electromagnetic, 1.0, 1.5, 0.0, 0), 0.04), "Normal-incidence reflectance");
	CHECK(approx(em_critical_angle(electromagnetic, 1.5, 1.0), std::asin(1.0 / 1.5)),
		  "EM critical angle");
	CHECK(approx(em_anti_reflection_thickness(electromagnetic, 1.5, 600.0e-9), 100.0e-9),
		  "EM anti-reflection thickness");
	CHECK(approx(em_rect_waveguide_cutoff(electromagnetic, 1, 0, 0.2, 0.1), mediumSpeed / 0.4),
		  "Rectangular waveguide cutoff");
	CHECK(approx(em_rect_waveguide_phase_vel(electromagnetic, 2.0, 1.0), mediumSpeed / std::sqrt(0.75)) &&
		  approx(em_rect_waveguide_group_vel(electromagnetic, 2.0, 1.0), mediumSpeed * std::sqrt(0.75)),
		  "Waveguide phase and group velocities");
	CHECK(approx(em_circ_waveguide_cutoff_te(electromagnetic, 1, 1, 0.1),
			  1.8412 * mediumSpeed / (2.0 * emPi * 0.1)), "Circular TE cutoff");
	CHECK(approx(em_circ_waveguide_cutoff_tm(electromagnetic, 0, 1, 0.1),
			  2.4048 * mediumSpeed / (2.0 * emPi * 0.1)), "Circular TM cutoff");
	CHECK(approx(em_coax_impedance(electromagnetic, 0.1, 0.2),
			  376.730313668 / (2.0 * emPi) * std::sqrt(1.0 / 2.5) * std::log(2.0)),
		  "Coaxial impedance");
	CHECK(approx(em_reflection_coefficient(electromagnetic, 75.0, 50.0), 0.2) &&
		  approx(em_reflection_coefficient(electromagnetic, 50.0, 50.0), 0.0),
		  "Transmission-line reflection coefficient");
	CHECK(approx(em_vswr(electromagnetic, -0.2), 1.5), "Standing-wave ratio");
	em_destroy(electromagnetic);

	const double expectedReynolds = 998.0 * 1.5 * 0.05 / 1.002e-3;
	CHECK(approx(fluid_reynolds_number(998.0, 1.5, 0.05, 1.002e-3), expectedReynolds),
		  "Fluid Reynolds number");

	NP_Handle nuclear = np_create();
	CHECK(nuclear != nullptr, "Nuclear handle creation");
	const double bindingEnergy = np_binding_energy_semf(nuclear, 26, 56);
	CHECK(std::isfinite(bindingEnergy) && bindingEnergy > 0.0, "Nuclear binding energy");
	double curveA[3] = {};
	double curveBinding[3] = {};
	CHECK(np_compute_semf_curve(nuclear, 5, curveA, curveBinding, 3) == 3,
		  "Nuclear SEMF curve truncated output length");
	CHECK(approx(curveA[0], 1.0) && approx(curveA[2], 3.0) &&
		  std::isfinite(curveBinding[1]),
		  "Nuclear SEMF curve output values");
	CHECK(approx(np_nuclear_radius(nuclear, 56), 1.2 * std::cbrt(56.0)),
		  "Nuclear radius");
	CHECK(approx(np_pairing_delta(nuclear, 100), 12.0 / 10.0), "Nuclear pairing delta");
	CHECK(np_binding_energy_semf(nuclear, 0, 56) == 0.0,
		  "Invalid nuclear charge returns zero binding energy");
	CHECK(approx(np_cherenkov_angle(nuclear, 0.9, 1.5),
				  std::acos(1.0 / (0.9 * 1.5))),
		  "Nuclear Cherenkov angle");
	CHECK(np_cherenkov_angle(nuclear, 0.5, 1.5) == 0.0,
		  "Sub-threshold Cherenkov angle");
	CHECK(std::isfinite(np_mass_excess(nuclear, 26, 56)),
		  "Nuclear mass excess");
	CHECK(std::isfinite(np_symmetry_energy(nuclear, 26, 56)) &&
		  np_symmetry_energy(nuclear, 26, 56) > 0.0,
		  "Nuclear symmetry energy");
	CHECK(np_alpha_half_life(nuclear, 2, 4, 0.0) == 1.0e30,
		  "Invalid alpha-decay domain");
	CHECK(approx(np_fermi_energy(nuclear, 56),
			  (197.3269804 * 197.3269804) *
			  std::pow(9.0 * 3.14159265358979323846 / 4.0, 2.0 / 3.0) /
			  (1.2 * 1.2 * 2.0 * 938.272046)),
		  "Nuclear Fermi energy");
	CHECK(approx(np_level_density_bethe(nuclear, 100, 10.0),
			  std::sqrt(3.14159265358979323846) / 12.0 *
			  std::exp(2.0 * std::sqrt((100.0 / 8.0) * (10.0 - 1.2))) /
			  (std::pow(100.0 / 8.0, 0.25) * std::pow(10.0 - 1.2, 1.25))),
		  "Nuclear Bethe level density");
	CHECK(approx(np_radiocarbon_age(nuclear, 0.5), 5730.0),
		  "Nuclear radiocarbon half-life");
	CHECK(approx(np_threshold_energy(nuclear, -2.0, 1.0, 2.0), 2.0),
		  "Nuclear threshold energy");
	CHECK(approx(np_four_factor(nuclear, 0.9, 0.8, 0.95, 1.1),
			  0.9 * 0.8 * 0.95 * 1.1),
		  "Nuclear four-factor formula");
	CHECK(approx(np_moderation_xi(nuclear, 2.0),
			  1.0 + 1.0 / 18.0 * std::log(1.0 / 3.0)),
		  "Nuclear moderation logarithmic decrement");
	CHECK(approx(np_quadrupole_moment(nuclear, 2, 8, 0.2),
			  3.0 / std::sqrt(5.0 * 3.14159265358979323846) * 2.0 *
			  2.4 * 2.4 * 0.2 * (1.0 + 0.36 * 0.2)), "Nuclear quadrupole moment");
	CHECK(approx(np_internal_conversion(nuclear, 10, 1, 1.0),
			  std::pow(1.0 / 137.035999084, 4.0) * 1000.0 *
			  std::pow(2.0 * 0.510998950, 1.5) * 2.0), "Internal conversion coefficient");
	CHECK(np_internal_conversion(nuclear, 10, 1, 0.0) == 0.0, "Internal conversion zero energy");
	CHECK(approx(np_q_value(nuclear, 26, 56, 20, 40, 26, 56, 20, 40), 0.0),
		  "Balanced nuclear reaction Q value");
	double attenuationX[4] = {-1.0, -1.0, -1.0, -1.0};
	double attenuationI[4] = {-1.0, -1.0, -1.0, -1.0};
	CHECK(np_compute_exponential_atten(nuclear, 2.0, 2.0, attenuationX, attenuationI, 3) == 3,
		  "Nuclear attenuation truncated length");
	CHECK(approx(attenuationX[0], 0.0) && approx(attenuationX[2], 0.02) &&
		  approx(attenuationI[0], 1.0) && approx(attenuationI[2], std::exp(-0.04)) &&
		  attenuationX[3] == -1.0 && attenuationI[3] == -1.0,
		  "Nuclear attenuation values and buffer boundary");
	double nuclearX[502];
	double nuclearY[502];
	const auto resetNuclearArrays = [&]() {
		std::fill_n(nuclearX, 502, -1.0);
		std::fill_n(nuclearY, 502, -1.0);
	};
	resetNuclearArrays();
	CHECK(np_compute_binding_curve(nuclear, nuclearX, nuclearY, 501) == 260,
		  "Nuclear binding curve full length");
	const double bindingHe4 = (15.56 * 4.0 - 17.23 * std::pow(4.0, 2.0 / 3.0) -
		0.7 * 2.0 / std::cbrt(4.0) + 12.0 / 2.0) / 4.0;
	CHECK(approx(nuclearX[3], 4.0) && approx(nuclearY[3], bindingHe4) &&
		  approx(nuclearX[259], 260.0) && nuclearX[260] == -1.0 && nuclearY[260] == -1.0,
		  "Nuclear binding curve values and full buffer boundary");
	resetNuclearArrays();
	CHECK(np_compute_binding_curve(nuclear, nuclearX, nuclearY, 3) == 3 &&
		  approx(nuclearX[2], 3.0) && nuclearX[3] == -1.0 && nuclearY[3] == -1.0,
		  "Nuclear binding curve truncated buffer");
	resetNuclearArrays();
	CHECK(np_compute_charge_density(nuclear, 8, nuclearX, nuclearY, 501) == 201,
		  "Nuclear charge density full length");
	const double chargeRadius = 2.4;
	const double chargeMax = chargeRadius + 6.0 * 0.545;
	CHECK(approx(nuclearX[0], 0.0) && approx(nuclearX[200], chargeMax) &&
		  approx(nuclearY[0], 0.17 / (1.0 + std::exp(-chargeRadius / 0.545))) &&
		  approx(nuclearY[200], 0.17 / (1.0 + std::exp(6.0))) &&
		  nuclearX[201] == -1.0 && nuclearY[201] == -1.0,
		  "Nuclear charge density endpoints and buffer boundary");
	resetNuclearArrays();
	CHECK(np_compute_beta_spectrum(nuclear, 10, 2.0, nuclearX, nuclearY, 501) == 199,
		  "Nuclear beta spectrum full length");
	const double betaElectronMass = 0.510998950;
	const double betaEnergy = 0.01 + betaElectronMass;
	const double betaMomentum = std::sqrt(betaEnergy * betaEnergy - betaElectronMass * betaElectronMass) /
		betaElectronMass;
	const double betaW = betaEnergy / betaElectronMass;
	const double betaEta = (1.0 / 137.035999084) * 10.0 * betaW / betaMomentum;
	const double nuclearPi = 3.14159265358979323846;
	const double betaFermi = 2.0 * nuclearPi * betaEta / (1.0 - std::exp(-2.0 * nuclearPi * betaEta));
	CHECK(approx(nuclearX[0], 0.01) && approx(nuclearX[198], 1.99) &&
		  approx(nuclearY[0], betaFermi * betaMomentum * betaW *
				 std::pow((2.0 + betaElectronMass) / betaElectronMass - betaW, 2.0)) &&
		  nuclearX[199] == -1.0 && nuclearY[199] == -1.0,
		  "Nuclear beta spectrum values and buffer boundary");
	resetNuclearArrays();
	CHECK(np_compute_rutherford(nuclear, 2, 10, 5.0, nuclearX, nuclearY, 3) == 3,
		  "Rutherford truncated length");
	const double firstTheta = nuclearPi * 4.0 / 200.0;
	CHECK(approx(nuclearX[0], 3.6) &&
		  approx(nuclearY[0], std::pow(2.0 * 10.0 * 1.4399764 / (4.0 * 5.0), 2.0) /
				 std::pow(std::sin(firstTheta / 2.0), 4.0)) &&
		  nuclearX[3] == -1.0 && nuclearY[3] == -1.0,
		  "Rutherford scattering values and buffer boundary");
	resetNuclearArrays();
	CHECK(np_compute_breit_wigner(nuclear, 10.0, 1.0, 0.5, 0.5, 0.5, 0.5, 0.0,
								  nuclearX, nuclearY, 501) == 401,
		  "Breit-Wigner full length");
	CHECK(approx(nuclearX[0], 5.0) && approx(nuclearX[200], 10.0) &&
		  approx(nuclearX[400], 15.0) &&
		  approx(nuclearY[200], nuclearPi * 197.3269804 * 197.3269804 /
				 (2.0 * 938.272046 * 10.0 * 100.0)) &&
		  nuclearX[401] == -1.0 && nuclearY[401] == -1.0,
		  "Breit-Wigner resonance and buffer boundary");
	resetNuclearArrays();
	CHECK(np_compute_neutrino_osc(nuclear, 0.0025, 0.8, 295.0, 5.0,
								 nuclearX, nuclearY, 501) == 500,
		  "Neutrino oscillation full length");
	const double neutrinoPhase = 1.27 * 0.0025 * 295.0 / 0.01;
	CHECK(approx(nuclearX[0], 0.01) && approx(nuclearX[499], 5.0) &&
		  approx(nuclearY[0], 1.0 - 0.8 * std::pow(std::sin(neutrinoPhase), 2.0)) &&
		  nuclearX[500] == -1.0 && nuclearY[500] == -1.0,
		  "Neutrino oscillation values and buffer boundary");
	resetNuclearArrays();
	CHECK(np_compute_bethe_bloch(nuclear, 1, 938.272046, 13, 26.98, 166.0,
							   nuclearX, nuclearY, 3) == 3,
		  "Bethe-Bloch truncated length");
	const double stoppingEnergy = 0.1 + 10000.0 / 300.0;
	const double stoppingGamma = 1.0 + stoppingEnergy / 938.272046;
	const double stoppingBeta2 = 1.0 - 1.0 / (stoppingGamma * stoppingGamma);
	const double electronRatio = 0.510998950 / 938.272046;
	const double stoppingTmax = 2.0 * 0.510998950 * stoppingBeta2 * stoppingGamma * stoppingGamma /
		(1.0 + 2.0 * stoppingGamma * electronRatio + electronRatio * electronRatio);
	const double stoppingArg = 2.0 * 0.510998950 * stoppingBeta2 * stoppingGamma * stoppingGamma *
		stoppingTmax / std::pow(166.0e-6, 2.0);
	const double expectedStopping = 0.307075 * 13.0 / (26.98 * stoppingBeta2) *
		(0.5 * std::log(stoppingArg) - stoppingBeta2);
	CHECK(approx(nuclearX[0], stoppingEnergy) && approx(nuclearY[0], expectedStopping) &&
		  nuclearX[3] == -1.0 && nuclearY[3] == -1.0,
		  "Bethe-Bloch stopping power and buffer boundary");
	resetNuclearArrays();
	CHECK(np_compute_charge_density(nuclear, 8, nuclearX, nuclearY, 0) == 0 &&
		  nuclearX[0] == -1.0 && nuclearY[0] == -1.0, "Nuclear zero-capacity output");
	np_destroy(nuclear);

	OP_Handle optics = op_create("integration-optics", 632.8e-9, 1.0);
	CHECK(optics != nullptr, "Optics handle creation");
	CHECK(approx(op_get_wavelength(optics), 632.8e-9), "Optics wavelength");
	CHECK(approx(op_snell_refraction(optics, 0.5, 1.0, 1.5),
				  std::asin(std::sin(0.5) / 1.5)),
		  "Snell refraction");
	CHECK(op_snell_refraction(optics, 1.2, 1.5, 1.0) == -1.0,
		  "Optics total internal reflection domain");
	CHECK(approx(op_critical_angle(optics, 1.5, 1.0), std::asin(1.0 / 1.5)),
		  "Optics critical angle");
	CHECK(approx(op_brewster_angle(optics, 1.0, 1.5), std::atan(1.5)),
		  "Optics Brewster angle");
	CHECK(approx(op_thin_lens(optics, 0.30, 0.10), 0.15), "Thin lens equation");
	CHECK(approx(op_magnification(optics, 0.30, 0.15), -0.5), "Optics magnification");
	CHECK(approx(op_spherical_mirror_focal(optics, 0.40), 0.20), "Spherical mirror focal length");
	CHECK(approx(op_airy_disk_radius(optics, 500.0e-9, 0.20, 0.05),
				  1.22 * 500.0e-9 * 0.20 / 0.05),
		  "Airy disk radius");
	CHECK(approx(op_chromatic_aberration(optics, 0.20, 50.0), -0.20 / 50.0),
		  "Chromatic aberration");
	CHECK(approx(op_anti_reflection_thickness(optics, 1.5, 600.0e-9),
				  600.0e-9 / (4.0 * 1.5)),
		  "Anti-reflection film thickness");
	CHECK(approx(op_cavity_fsr(optics, 0.10, 1.5), 299792458.0 / (2.0 * 1.5 * 0.10)),
		  "Optical cavity free spectral range");
	CHECK(approx(op_bragg_angle(optics, 500.0e-9, 2.0e-6),
				  std::asin(500.0e-9 / (2.0 * 2.0e-6))),
		  "Acousto-optic Bragg angle");
	CHECK(op_fresnel_number(optics, 1.0e-3, 500.0e-9, 0.10) == 20,
		  "Fresnel number");
	CHECK(approx(op_cavity_stability(optics, 0.10, 0.20, 0.30),
				  (1.0 - 0.10 / 0.20) * (1.0 - 0.10 / 0.30)),
		  "Optical cavity stability");
	CHECK(approx(op_gaussian_rayleigh(optics, 1.0e-3, 500.0e-9),
				  3.14159265358979323846 * 1.0e-6 / 500.0e-9),
		  "Gaussian beam Rayleigh range");
	const double fresnelTransmittedCosine =
		std::sqrt(1.0 - std::pow(std::sin(0.4) / 1.5, 2.0));
	CHECK(approx(op_fresnel_rs(optics, 0.4, 1.0, 1.5),
			  std::pow((std::cos(0.4) - 1.5 * fresnelTransmittedCosine) /
						(std::cos(0.4) + 1.5 * fresnelTransmittedCosine), 2.0)),
		  "Fresnel s-polarized reflectance");
	CHECK(approx(op_fresnel_rp(optics, 0.4, 1.0, 1.5),
			  std::pow((1.5 * std::cos(0.4) - fresnelTransmittedCosine) /
						(1.5 * std::cos(0.4) + fresnelTransmittedCosine), 2.0)),
		  "Fresnel p-polarized reflectance");
	CHECK(approx(op_thick_lens_focal_length(optics, 1.5, 0.20, -0.20, 0.01),
			  1.0 / (0.5 / 0.20 + (-0.5 / -0.20) -
					(0.01 / 1.5) * (0.5 / 0.20) * (-0.5 / -0.20))),
		  "Thick lens focal length");
	CHECK(approx(op_mirror_equation(optics, 0.30, 0.10), 0.15),
		  "Mirror equation");
	CHECK(approx(op_prism_deviation(optics, 0.3, 1.5, 0.2),
			  0.3 + std::asin(1.5 * std::sin(0.2 -
						 std::asin(std::sin(0.3) / 1.5))) - 0.2),
		  "Prism deviation");
	CHECK(approx(op_prism_min_deviation(optics, 1.5, 0.2),
			  2.0 * std::asin(1.5 * std::sin(0.1)) - 0.2),
		  "Minimum prism deviation");
	CHECK(approx(op_telescope_magnification(optics, 1.0, 0.05), -20.0) &&
		  approx(op_microscope_na(optics, 1.5, 0.4), 1.5 * std::sin(0.4)) &&
		  approx(op_rayleigh_criterion(optics, 500.0e-9, 0.5), 0.61e-6),
		  "Optical instrument formulas");
	CHECK(approx(op_fiber_na(optics, 1.5, 1.4), std::sqrt(1.5 * 1.5 - 1.4 * 1.4)) &&
		  approx(op_fiber_v_number(optics, 4.0e-6, 0.5, 1.0e-6),
				 2.0 * 3.14159265358979323846 * 4.0e-6 * 0.5 / 1.0e-6) &&
		  op_fiber_num_modes(optics, 4.0) == 8,
		  "Optical fiber formulas");
	CHECK(approx(op_optical_path_length(optics, 1.5, 0.20), 0.30) &&
		  approx(op_eikonal_phase(optics, 0.30, 600.0e-9),
				 2.0 * 3.14159265358979323846 * 0.30 / 600.0e-9),
		  "Optical path and eikonal phase");
	CHECK(approx(op_grating_resolving_power(optics, 2, 500), 1000.0) &&
		  approx(op_michelson_visibility(optics, 3.0, 1.0), 0.5) &&
		  approx(op_fabry_perot_finesse(optics, 0.25),
				 3.14159265358979323846 * 0.5 / 0.75),
		  "Interferometer formulas");
	CHECK(approx(op_degree_of_coherence(optics, 0.5, 1.0),
			  std::exp(-3.14159265358979323846 * 0.25)) &&
		  approx(op_malus_law(optics, 4.0, 3.14159265358979323846 / 3.0), 1.0),
		  "Coherence and Malus-law formulas");
	CHECK(approx(op_mz_output1(optics, 0.0), 1.0) &&
		  approx(op_mz_output2(optics, 0.0), 0.0),
		  "Mach-Zehnder outputs");
	CHECK(approx(op_get_n_medium(optics), 1.0), "Optics medium index getter");
	const double slitAngle = std::asin(0.25);
	const double pi = 3.14159265358979323846;
	CHECK(approx(op_single_slit_intensity(optics, 0.0, 1.0, 1.0), 1.0) &&
		  approx(op_single_slit_intensity(optics, slitAngle, 1.0, 1.0),
				 std::pow(std::sin(pi / 4.0) / (pi / 4.0), 2.0)), "Single slit intensity");
	CHECK(approx(op_double_slit_intensity(optics, 0.0, 1.0, 1.0, 1.0), 4.0) &&
		  approx(op_double_slit_intensity(optics, slitAngle, 1.0, 1.0, 1.0),
				 4.0 * std::pow(std::sin(pi / 4.0) / (pi / 4.0), 2.0) * 0.5),
		  "Double slit intensity");
	CHECK(approx(op_grating_intensity(optics, 0.0, 1.0, 1.0, 1.0, 4), 1.0),
		  "Grating central maximum");
	CHECK(approx(op_airy_intensity(optics, 0.0, 1.0, 1.0), 1.0), "Airy central maximum");
	CHECK(approx(op_michelson_intensity(optics, 0.0, 1.0), 1.0) &&
		  std::abs(op_michelson_intensity(optics, 0.25, 1.0)) < 1.0e-12,
		  "Michelson bright and dark fringes");
	CHECK(approx(op_fabry_perot_transmission(optics, 0.0, 3.0), 1.0) &&
		  approx(op_fabry_perot_transmission(optics, pi, 3.0), 0.25),
		  "Fabry-Perot transmission extrema");
	CHECK(approx(op_fabry_perot_fsr(optics, 0.10, 1.5), lightSpeed / 0.30),
		  "Fabry-Perot free spectral range");
	CHECK(approx(op_talbot_distance(optics, 2.0e-6, 500.0e-9), 16.0e-6),
		  "Talbot distance");
	const double waist = 1.0e-3;
	const double wavelength = 500.0e-9;
	const double rayleighRange = pi * waist * waist / wavelength;
	CHECK(approx(op_gaussian_waist(optics, waist, 0.0, wavelength), waist) &&
		  approx(op_gaussian_waist(optics, waist, rayleighRange, wavelength), waist * std::sqrt(2.0)) &&
		  approx(op_gaussian_divergence(optics, waist, wavelength), wavelength / (pi * waist)) &&
		  approx(op_gaussian_gouy_phase(optics, rayleighRange, rayleighRange), pi / 4.0) &&
		  approx(op_gaussian_intensity(optics, waist, waist), std::exp(-2.0)),
		  "Gaussian beam propagation and intensity");
	CHECK(approx(op_birefringent_phase_delay(optics, 0.1, 2.0e-6, wavelength), 0.8 * pi) &&
		  approx(op_walkoff_angle(optics, 1.4, 1.4, pi / 4.0), 0.0) &&
		  approx(op_optical_rotation(optics, 2.0, 0.5, 0.3), 0.3) &&
		  approx(op_faraday_rotation(optics, 4.0, 0.2, 0.3), 0.24),
		  "Birefringence and optical rotation");
	CHECK(approx(op_laser_threshold_gain(optics, 0.2, 0.5), 0.2) &&
		  approx(op_laser_slope_efficiency(optics, 2.0, 3.0, 4.0, 12.0), 0.5) &&
		  approx(op_mode_lock_rep_rate(optics, 0.5, 1.5), lightSpeed / 1.5) &&
		  approx(op_mode_lock_pulse_duration(optics, 2.0e12), 0.5e-12),
		  "Laser threshold and mode-locking formulas");
	CHECK(approx(op_shg_phase_mismatch(optics, 1.5, 1.6, wavelength), 0.4 * pi / wavelength) &&
		  approx(op_shg_phase_mismatch(optics, 1.5, 1.5, wavelength), 0.0) &&
		  approx(op_shg_coherence_length(optics, 1.5, 1.6, wavelength), wavelength / 0.4),
		  "Second-harmonic phase mismatch and coherence length");
	CHECK(approx(op_kerr_index(optics, 1.5, 2.0e-20, 1.0e12), 1.5 + 2.0e-8) &&
		  approx(op_b_integral(optics, 2.0e-20, 1.0e12, 0.1, wavelength),
				 2.0 * pi * 2.0e-20 * 1.0e12 * 0.1 / wavelength) &&
		  approx(op_critical_power(optics, wavelength, 1.5, 2.0e-20),
				 3.77 * wavelength * wavelength / (8.0 * pi * 1.5 * 2.0e-20)),
		  "Kerr index, phase and critical power");
	CHECK(approx(op_opa_gain(optics, 2.0, 0.5, 0.0), std::pow(std::cosh(1.0), 2.0)) &&
		  approx(op_opa_gain(optics, 2.0, 0.5, 4.0), 1.0) &&
		  approx(op_opa_gain(optics, 2.0, 0.5, 5.0), 1.0) &&
		  approx(op_opa_idler_wavelength(optics, 500.0e-9, 750.0e-9), 1.5e-6),
		  "Optical parametric amplification and idler wavelength");
	const double halfWaveVoltage = wavelength * 1.0e-3 / (8.0 * 2.0e-12);
	CHECK(approx(op_half_wave_voltage(optics, wavelength, 2.0, 2.0e-12, 1.0e-3), halfWaveVoltage) &&
		  approx(op_pockels_phase_shift(optics, 2.0, 2.0e-12, halfWaveVoltage, wavelength, 1.0e-3), pi) &&
		  approx(op_ao_frequency_shift(optics, 80.0e6), 80.0e6),
		  "Electro-optic half-wave voltage and acousto-optic shift");
	CHECK(approx(op_poisson_distribution(2.0, 0), std::exp(-2.0)) &&
		  approx(op_poisson_distribution(2.0, 3), 4.0 * std::exp(-2.0) / 3.0) &&
		  approx(op_thermal_distribution(2.0, 0), 1.0 / 3.0) &&
		  approx(op_thermal_distribution(2.0, 2), 4.0 / 27.0) &&
		  approx(op_mandel_q(optics, 1.0, 2.0), -0.5),
		  "Photon statistics distributions and Mandel Q");
	CHECK(approx(op_hbt_g2_thermal(optics, 0.0, 2.0), 2.0) &&
		  approx(op_hbt_g2_thermal(optics, -2.0, 2.0), 1.0 + std::exp(-2.0)) &&
		  approx(op_hom_dip(optics, 0.0, 2.0), 0.0) &&
		  approx(op_hom_dip(optics, 2.0, 2.0), 1.0 - std::exp(-1.0)) &&
		  approx(op_mz_sensitivity(optics, pi / 2.0), 1.0),
		  "Photon correlations and interferometer sensitivity");
	CHECK(approx(op_squeezed_variance_x(optics, 0.5), std::exp(-1.0)) &&
		  approx(op_squeezed_variance_p(optics, 0.5), std::exp(1.0)) &&
		  approx(op_squeezed_db(optics, 0.5), -10.0 / std::log(10.0)),
		  "Squeezed-light variances and decibels");
	const double qber = 0.05;
	const double binaryEntropy = -qber * std::log2(qber) - (1.0 - qber) * std::log2(1.0 - qber);
	CHECK(approx(op_bb84_qber(optics, 0.02, 0.03), qber) &&
		  approx(op_bb84_secure_key_rate(optics, 0.0, 1.0), 1.0) &&
		  approx(op_bb84_secure_key_rate(optics, qber, 1.0), 1.0 - 2.0 * binaryEntropy) &&
		  approx(op_bb84_secure_key_rate(optics, 0.11, 1.0), 0.0),
		  "BB84 error and secure-rate threshold");
	const double sppFactor = std::sqrt(2.0);
	CHECK(approx(op_spp_wavevector(optics, 3.0e15, -2.0, 1.0), 3.0e15 * sppFactor / lightSpeed) &&
		  approx(op_spr_resonance_angle(optics, 2.0, -2.0, 1.0), pi / 4.0),
		  "Surface plasmon wavevector and resonance angle");
	CHECK(approx(op_metamaterial_epsilon(optics, 2.0, 3.0, 1.0), -0.8) &&
		  approx(op_metamaterial_mu(optics, 2.0, 1.0, 0.6, 0.1), 0.8) &&
		  approx(op_metamaterial_n(optics, -4.0, -1.0), -2.0) &&
		  approx(op_metamaterial_n(optics, 4.0, 1.0), 2.0) &&
		  approx(op_negative_refraction(optics, pi / 6.0, 1.0, -1.0), -pi / 6.0),
		  "Metamaterial response and negative refraction");
	op_destroy(optics);

	CHECK(approx(ss_lattice_spacing(1, 0, 0, 5.0), 5.0), "Solid-state lattice spacing");
	CHECK(approx(ss_bragg_angle(2.5e-10, 1.0e-10, 1),
				  std::asin(1.0e-10 / (2.0 * 2.5e-10))),
		  "Bragg angle");
	CHECK(approx(ss_thermal_conductivity(10.0, 200.0, 1.0e-6),
				  (1.0 / 3.0) * 10.0 * 200.0 * 1.0e-6),
		  "Solid-state kinetic thermal conductivity");
	CHECK(approx(ss_landauer_conductance(0.5, 2),
				  0.5 * 2.0 * ss_conductance_quantum()),
		  "Landauer conductance");
	const double solidStateHbar = 1.054571817e-34;
	CHECK(approx(ss_free_electron_energy(2.0e9, 9.1093837015e-31),
				  solidStateHbar * solidStateHbar * 4.0e18 /
				  (2.0 * 9.1093837015e-31)),
		  "Free-electron energy");
	CHECK(approx(ss_fermi_energy(1.0e28, 9.1093837015e-31),
				  (solidStateHbar * solidStateHbar /
				   (2.0 * 9.1093837015e-31)) *
				  std::pow(3.0 * 3.14159265358979323846 * 3.14159265358979323846 * 1.0e28,
							2.0 / 3.0)),
		  "Fermi energy");
	CHECK(approx(ss_flux_quantum(), 6.62607015e-34 / (2.0 * 1.602176634e-19)),
		  "Superconducting flux quantum");
	CHECK(approx(ss_bcs_gap(0.0, 2.0e-22, 10.0), 2.0e-22),
		  "Zero-temperature BCS gap");
	CHECK(approx(ss_bcs_gap(10.0, 2.0e-22, 10.0), 0.0),
		  "Critical-temperature BCS gap");
	CHECK(approx(ss_monatomic_dispersion(1.0, 0.5, 4.0, 2.0),
			  2.0 * std::sqrt(2.0) * std::abs(std::sin(0.25))),
		  "Monatomic lattice dispersion");
	CHECK(approx(ss_fermi_dirac(0.0, 0.0, 300.0), 0.5),
		  "Solid-state Fermi-Dirac midpoint");
	CHECK(approx(ss_tight_binding_1d(2.0, 0.5, 3.14159265358979323846, 1.0), 3.0),
		  "Tight-binding one-dimensional energy");
	CHECK(approx(ss_diode_current(1.0e-12, 0.0, 300.0, 1.0), 0.0),
		  "Ideal diode zero-bias current");
	CHECK(approx(ss_hall_coefficient(1.0e28, 1.602176634e-19),
			  -1.0 / (1.0e28 * 1.602176634e-19)),
		  "Hall coefficient");
	CHECK(approx(ss_josephson_ac_freq(1.0e-6),
			  2.0 * 1.602176634e-19 * 1.0e-6 / 6.62607015e-34),
		  "Josephson AC frequency");
	CHECK(approx(ss_ising_2d_tc(1.0),
			  2.0 / (1.380649e-23 * std::log(1.0 + std::sqrt(2.0)))),
		  "Two-dimensional Ising critical temperature");
	CHECK(approx(ss_bcs_tc(1.0e-21, 2.0e46, 1.0e-47),
			  1.13e-21 * std::exp(-1.0 / (2.0e46 * 1.0e-47))),
		  "BCS critical temperature");
	CHECK(approx(ss_brillouin_function(0.0, 0.5), 0.0),
		  "Brillouin zero-field limit");
	CHECK(approx(ss_curie_weiss_temperature(1.0e28, 0.5, 2.0, 1.0),
			  (1.0e28 * 1.2566370614e-6 * 4.0 *
			   9.2740100783e-24 * 9.2740100783e-24 * 0.5 * 1.5) /
			  (3.0 * 1.380649e-23)),
		  "Curie-Weiss temperature");

	TD_Handle thermo = td_create("integration-thermo", 1.0, 300.0);
	CHECK(thermo != nullptr, "Thermodynamics handle creation");
	CHECK(approx(td_get_nMoles(thermo), 1.0), "Thermodynamics amount getter");
	CHECK(approx(td_get_temperature(thermo), 300.0), "Thermodynamics temperature");
	td_set_temperature(thermo, 350.0);
	CHECK(approx(td_get_temperature(thermo), 350.0), "Thermodynamics temperature setter");
	td_set_temperature(thermo, 300.0);
	CHECK(approx(td_ideal_gas_pressure(thermo, 1.0), 8.31446261815324 * 300.0),
		  "Ideal-gas pressure");
	CHECK(approx(td_ideal_gas_volume(thermo, 8.31446261815324 * 300.0), 1.0),
		  "Ideal-gas volume");
	CHECK(approx(td_isothermal_work(thermo, 1.0, 2.0),
				  8.31446261815324 * 300.0 * std::log(2.0)),
		  "Isothermal work");
	CHECK(approx(td_carnot_efficiency(thermo, 600.0, 300.0), 0.5),
		  "Carnot efficiency");
	CHECK(approx(td_otto_efficiency(thermo, 8.0, 1.4),
				  1.0 - std::pow(1.0 / 8.0, 0.4)),
		  "Otto efficiency");
	CHECK(approx(td_entropy_change_isothermal(thermo, 1200.0, 300.0), 4.0),
		  "Isothermal entropy change");
	CHECK(approx(td_helmholtz_free_energy(thermo, 5000.0, 300.0, 10.0), 2000.0),
		  "Helmholtz free energy");
	CHECK(approx(td_enthalpy(thermo, 5000.0, 100000.0, 0.01), 6000.0),
		  "Thermodynamic enthalpy");
	CHECK(approx(td_adiabatic_index(thermo, 3), 5.0 / 3.0),
		  "Ideal-gas adiabatic index");
	CHECK(approx(td_boltzmann_probability(thermo, 2.0 * 1.380649e-23 * 300.0, 300.0),
				  std::exp(-2.0)),
		  "Boltzmann probability");
	CHECK(approx(td_fermi_dirac_distribution(thermo, 2.0 * 1.380649e-23 * 300.0,
								  0.0, 300.0),
				  1.0 / (std::exp(2.0) + 1.0)),
		  "Fermi-Dirac distribution");
	CHECK(approx(td_isobaric_work(thermo, 100000.0, 1.0, 1.5), 50000.0),
		  "Isobaric work");
	CHECK(approx(td_internal_energy_change(thermo, 20.0, 300.0, 350.0), 1000.0),
		  "Internal energy change");
	CHECK(approx(td_gibbs_free_energy(thermo, 5000.0, 300.0, 10.0), 2000.0),
		  "Gibbs free energy");
	CHECK(approx(td_equipartition_energy(thermo, 4, 300.0),
				  0.5 * 4.0 * 1.380649e-23 * 300.0),
		  "Equipartition energy");
	CHECK(approx(td_planck_peak_wavelength(thermo, 300.0), 2.897771955e-3 / 300.0),
		  "Planck peak wavelength");
	CHECK(approx(td_brownian_msd(thermo, 2.0e-12, 0.50), 2.0e-12),
		  "Brownian mean-square displacement");
	const double boltzmann = 1.380649e-23;
	CHECK(approx(td_bose_einstein_occupation(thermo, 2.0 * boltzmann * 300.0, 0.0, 300.0),
				  1.0 / (std::exp(2.0) - 1.0)),
		  "Bose-Einstein occupation");
	CHECK(approx(td_vdw_pressure(thermo, 1.0, 1.0, 0.01),
			  8.31446261815324 * 300.0 / 0.99 - 1.0),
		  "Van der Waals pressure");
	CHECK(approx(td_vdw_critical_temperature(thermo, 1.0, 0.01),
			  8.0 / (27.0 * 8.31446261815324 * 0.01)),
		  "Van der Waals critical temperature");
	CHECK(approx(td_maxwell_rms_speed(thermo, 4.65e-26, 300.0),
			  std::sqrt(3.0 * boltzmann * 300.0 / 4.65e-26)),
		  "Maxwell RMS speed");
	CHECK(approx(td_diesel_efficiency(thermo, 8.0, 2.0, 1.4),
			  1.0 - (std::pow(2.0, 1.4) - 1.0) /
			  (1.4 * (2.0 - 1.0) * std::pow(8.0, 0.4))),
		  "Diesel efficiency");
	CHECK(approx(td_entropy_change_ideal_gas(thermo, 20.0, 300.0, 600.0, 1.0, 2.0),
			  20.0 * std::log(2.0) + 8.31446261815324 * std::log(2.0)),
		  "Ideal-gas entropy change");
	CHECK(approx(td_clausius_clapeyron_slope(thermo, 40000.0, 300.0, 0.01),
			  40000.0 / (300.0 * 0.01)),
		  "Clausius-Clapeyron slope");
	CHECK(approx(td_vapor_pressure_cc(thermo, 100000.0, 40000.0, 300.0, 300.0), 100000.0),
		  "Clausius-Clapeyron reference pressure");
	CHECK(approx(td_joule_thomson_inversion_temp(thermo, 1.0, 0.01),
			  2.0 / (8.31446261815324 * 0.01)),
		  "Joule-Thomson inversion temperature");
	CHECK(approx(td_chemical_potential_ideal_gas(thermo, 100.0, 300.0, 101325.0, 101325.0),
			  100.0),
		  "Ideal-gas chemical potential reference pressure");
	CHECK(approx(td_thermal_wavelength(thermo, 300.0, 4.65e-26),
			  std::sqrt(2.0 * 3.14159265358979323846 * 1.0545718e-34 *
						 1.0545718e-34 / (4.65e-26 * boltzmann * 300.0))),
		  "Thermal de Broglie wavelength");
	CHECK(approx(td_stefan_boltzmann_power(thermo, 300.0, 2.0),
			  5.670374419e-8 * 2.0 * std::pow(300.0, 4.0)),
		  "Stefan-Boltzmann power");
	CHECK(approx(td_wien_displacement_law(thermo, 300.0), 2.897771955e-3 / 300.0),
		  "Wien displacement law");
	CHECK(approx(td_rayleigh_jeans(thermo, 1.0e12, 300.0),
			  2.0 * 1.380649e-23 * 300.0 * 1.0e24 /
			  (299792458.0 * 299792458.0)),
		  "Rayleigh-Jeans spectral radiance");
	const double thermoPi = 3.14159265358979323846;
	const double thermoHbar = 1.0545718e-34;
	const double electronCharge = 1.602176634e-19;
	const double gasConstant = 8.31446261815324;
	const double particleMass = 9.1093837015e-31;
	const double density = 1.0e28;
	const double fermiEnergy = thermoHbar * thermoHbar / (2.0 * particleMass) *
		std::pow(3.0 * thermoPi * thermoPi * density, 2.0 / 3.0);
	CHECK(approx(td_fermi_energy(thermo, density, particleMass), fermiEnergy) &&
		  approx(td_fermi_dirac_occupation(thermo, boltzmann * 300.0, 0.0, 300.0),
				 1.0 / (std::exp(1.0) + 1.0)) &&
		  approx(td_fermi_dirac_occupation(thermo, 0.0, 0.0, 300.0), 0.5),
		  "Fermi gas energy and occupation");
	CHECK(approx(td_bec_critical_temperature(thermo, density, particleMass),
				 2.0 * thermoPi * thermoHbar * thermoHbar / (particleMass * boltzmann) *
				 std::pow(density / 2.612, 2.0 / 3.0)) &&
		  approx(td_bec_condensate_fraction(thermo, 0.0, 100.0), 1.0) &&
		  approx(td_bec_condensate_fraction(thermo, 25.0, 100.0), 0.875) &&
		  approx(td_bec_condensate_fraction(thermo, 100.0, 100.0), 0.0) &&
		  approx(td_bec_condensate_fraction(thermo, 125.0, 100.0), 0.0),
		  "Bose condensation temperature and phase boundary");
	CHECK(approx(td_sommerfeld_energy(thermo, fermiEnergy, 300.0),
				 0.6 * fermiEnergy * (1.0 + 5.0 * thermoPi * thermoPi * boltzmann * boltzmann *
				 300.0 * 300.0 / (12.0 * fermiEnergy * fermiEnergy))) &&
		  approx(td_sommerfeld_specific_heat(thermo, fermiEnergy, 300.0),
				 thermoPi * thermoPi * boltzmann * boltzmann * 300.0 / (2.0 * fermiEnergy)),
		  "Sommerfeld energy and heat capacity");
	const double radiationDensity = 4.0 * 5.670374419e-8 * std::pow(300.0, 4.0) / 299792458.0;
	CHECK(approx(td_photon_gas_energy(thermo, 300.0, 2.0), 2.0 * radiationDensity) &&
		  approx(td_photon_gas_pressure(thermo, 300.0), radiationDensity / 3.0) &&
		  approx(td_chandrasekhar_mass(thermo, 2.0), 1.44 * 1.989e30 / 4.0),
		  "Photon gas and Chandrasekhar mass");
	CHECK(approx(td_einstein_solid_multiplicity(thermo, 2, 3), 6.0) &&
		  approx(td_einstein_solid_multiplicity(thermo, 0, 3), 1.0) &&
		  approx(td_einstein_specific_heat(thermo, 300.0, 300.0),
				 3.0 * gasConstant * std::exp(1.0) / std::pow(std::exp(1.0) - 1.0, 2.0)),
		  "Einstein solid multiplicity and specific heat");
	CHECK(approx(td_brillouin_function(thermo, 0.5, 0.0), 0.0) &&
		  approx(td_brillouin_function(thermo, 0.5, 0.7), std::tanh(0.7)) &&
		  approx(td_curie_law_susceptibility(thermo, 2.0, 4.0), 0.5) &&
		  approx(td_ising_2d_onsager_tc(thermo, boltzmann),
				 2.0 / std::log(1.0 + std::sqrt(2.0))),
		  "Paramagnetic and Ising references");
	CHECK(approx(td_mean_free_path(thermo, 2.0e25, 5.0e-20), 1.0e-6) &&
		  approx(td_knudsen_number(thermo, 1.0e-6, 1.0e-3), 1.0e-3) &&
		  approx(td_fick_flux(thermo, 2.0e-9, 3.0), -6.0e-9) &&
		  approx(td_diffusion_length(thermo, 2.0e-9, 4.0), std::sqrt(16.0e-9)),
		  "Mean free path and diffusion transport");
	CHECK(approx(td_drude_conductivity(thermo, density, 1.0e-14, particleMass),
				 density * electronCharge * electronCharge * 1.0e-14 / particleMass) &&
		  approx(td_lorenz_number(thermo), thermoPi * thermoPi * boltzmann * boltzmann /
				 (3.0 * electronCharge * electronCharge)),
		  "Drude conductivity and Lorenz number");
	CHECK(approx(td_thermal_conductivity_kinetic(thermo, 3.0, 4.0, 0.5), 2.0) &&
		  approx(td_fourier_heat_flux(thermo, 2.0, -3.0), 6.0) &&
		  approx(td_viscosity_kinetic(thermo, 3.0, 2.0, 4.0, 0.5), 4.0) &&
		  approx(td_reynolds_number(thermo, 2.0, 3.0, 4.0, 0.5), 48.0),
		  "Heat conduction, viscosity and Reynolds number");
	CHECK(approx(td_einstein_relation(thermo, 300.0, 1.0e-3, 1.0e-6),
				 boltzmann * 300.0 / (6.0 * thermoPi * 1.0e-3 * 1.0e-6)) &&
		  approx(td_langevin_msd(thermo, 300.0, 2.0, 1.0, 2.0),
				 2.0 * boltzmann * 300.0 * (2.0 - 2.0 * (1.0 - std::exp(-1.0)))) &&
		  approx(td_langevin_msd(thermo, 300.0, 2.0, 1.0, 0.0), 0.0),
		  "Stokes-Einstein diffusion and Langevin displacement");
	CHECK(approx(td_landau_free_energy(thermo, 2.0, -1.0, 2.0, 3.0), 9.0) &&
		  approx(td_landau_order_parameter(thermo, -2.0, 8.0), 0.5) &&
		  approx(td_landau_order_parameter(thermo, 0.0, 8.0), 0.0) &&
		  approx(td_mean_field_beta(thermo), 0.5) &&
		  approx(td_mean_field_gamma(thermo), 1.0),
		  "Landau order and mean-field exponents");
	CHECK(approx(td_equilibrium_constant(thermo, 0.0, 300.0), 1.0) &&
		  approx(td_equilibrium_constant(thermo, gasConstant * 300.0, 300.0), std::exp(-1.0)) &&
		  approx(td_gibbs_paradox_correction(thermo, 1, 1), 2.0 * boltzmann * std::log(2.0)),
		  "Chemical equilibrium and entropy of mixing");
	td_destroy(thermo);

	return true;
}

bool test_coupling_api()
{
	constexpr double mu0 = 1.2566370614e-6;
	constexpr double hbar = 1.0545718e-34;
	constexpr double electronCharge = 1.602176634e-19;
	constexpr double pi = 3.14159265358979323846;

	const double rho = 998.0;
	const double velocity = 1.5;
	const double length = 0.05;
	const double viscosity = 1.002e-3;
	const double heatCapacity = 4181.0;
	const double conductivity = 0.6;
	const double reynolds = rho * velocity * length / viscosity;
	const double prandtl = heatCapacity * viscosity / conductivity;
	const double expectedNusselt = 0.332 * std::sqrt(reynolds) * std::cbrt(prandtl);

	CHECK(approx(pe_nusselt_flat_plate(rho, velocity, length, viscosity,
									   heatCapacity, conductivity), expectedNusselt),
		  "Nusselt coupling");
	CHECK(approx(pe_convective_heat_flux(10.0, 0.5, 0.1, 300.0, 280.0), 1000.0),
		  "convective heat flux coupling");

	const double expectedSkinDepth = std::sqrt(2.0 / (mu0 * 5.8e7 * 1.0e9));
	CHECK(approx(pe_skin_depth(5.8e7, 1.0e9), expectedSkinDepth), "skin-depth coupling");

	const double mass = 0.067 * 9.1093837015e-31;
	const double dotSize = 5.0e-9;
	const double expectedConfinement = (pi * pi * hbar * hbar) /
									   (2.0 * mass * dotSize * dotSize);
	CHECK(approx(pe_quantum_dot_confinement(mass, dotSize), expectedConfinement),
		  "quantum-dot coupling");

	const double density = 8.5e28;
	const double expectedFermi = (hbar * hbar / (2.0 * 9.1093837015e-31)) *
								 std::pow(3.0 * pi * pi * density, 2.0 / 3.0);
	CHECK(approx(pe_fermi_energy(density, 9.1093837015e-31), expectedFermi),
		  "Fermi-energy coupling");

	CHECK(std::isfinite(pe_gamow_peak(1.5e7, 1, 1, 0.5 * 1.67262192369e-27)) &&
		  pe_gamow_peak(1.5e7, 1, 1, 0.5 * 1.67262192369e-27) > 0.0,
		  "Gamow-peak coupling");

	CHECK(approx(pe_terminal_velocity(1000.0, 1.0, 0.01, 1.0e-3),
				 (2.0 / 9.0) * 999.0 * 9.80665 * 0.01 * 0.01 / 1.0e-3),
		  "terminal-velocity coupling");
	CHECK(approx(pe_fission_thermal_power(1.0e12, 200.0),
				 1.0e12 * 200.0 * 1.0e6 * electronCharge),
		  "fission-power coupling");

	return true;
}

void write_async_result(void* data)
{
	static_cast<std::atomic<int>*>(data)->store(42, std::memory_order_release);
}

bool test_async_api()
{
	pe_async_init(2);
	std::atomic<int> result{0};
	const PE_TaskId task = pe_submit_async(write_async_result, &result);

	pe_task_wait(task);
	CHECK(pe_task_ready(task) == 1, "completed async task");
	CHECK(result.load(std::memory_order_acquire) == 42, "async callback result");

	pe_async_shutdown();
	return true;
}

} // namespace

int main()
{
	if (!test_metadata() ||
		!test_quantum_states() ||
		!test_module_apis() ||
		!test_coupling_api() ||
		!test_async_api()) {
		return 1;
	}

	std::cout << "PhysicsEngine integration tests passed.\n";
	return 0;
}
