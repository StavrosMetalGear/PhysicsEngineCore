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

bool test_module_apis()
{
	QP_Handle quantum = qp_create("integration-quantum", 9.1093837015e-31, 1.0e-10, 1);
	CHECK(quantum != nullptr, "Quantum handle creation");
	const double quantumEnergy = qp_energy_1d_box(quantum, 3);
	CHECK(std::isfinite(quantumEnergy) && quantumEnergy > 0.0, "Quantum energy");
	const double hbar = 1.0545718e-34;
	const double deltaPotential = 1.0e-18;
	CHECK(approx(qp_delta_potential_energy(quantum, deltaPotential),
				  -9.1093837015e-31 * deltaPotential * deltaPotential /
				  (2.0 * hbar * hbar)),
		  "Quantum delta-potential energy");
	qp_destroy(quantum);

	CP_Handle classical = cp_create("integration-classical", 2.0, 1.0, 3);
	CHECK(classical != nullptr, "Classical handle creation");
	CHECK(approx(cp_getMass(classical), 2.0), "Classical mass");
	CHECK(approx(cp_projectileX(classical, 20.0, 0.5, 2.0), 20.0 * std::cos(0.5) * 2.0),
		  "Classical projectile position");
	cp_destroy(classical);

	EM_Handle electromagnetic = em_create("integration-em", 2.5, 1.0, 0.0);
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
	CHECK(approx(em_ohm_law(electromagnetic, 12.0, 4.0), 3.0), "Ohm law current");
	CHECK(approx(em_wave_speed_in_medium(electromagnetic),
				  299792458.0 / std::sqrt(2.5)),
		  "Wave speed in medium");
	em_destroy(electromagnetic);

	const double expectedReynolds = 998.0 * 1.5 * 0.05 / 1.002e-3;
	CHECK(approx(fluid_reynolds_number(998.0, 1.5, 0.05, 1.002e-3), expectedReynolds),
		  "Fluid Reynolds number");

	NP_Handle nuclear = np_create();
	CHECK(nuclear != nullptr, "Nuclear handle creation");
	const double bindingEnergy = np_binding_energy_semf(nuclear, 26, 56);
	CHECK(std::isfinite(bindingEnergy) && bindingEnergy > 0.0, "Nuclear binding energy");
	CHECK(approx(np_nuclear_radius(nuclear, 56), 1.2 * std::cbrt(56.0)),
		  "Nuclear radius");
	CHECK(approx(np_pairing_delta(nuclear, 100), 12.0 / 10.0), "Nuclear pairing delta");
	CHECK(np_binding_energy_semf(nuclear, 0, 56) == 0.0,
		  "Invalid nuclear charge returns zero binding energy");
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

	TD_Handle thermo = td_create("integration-thermo", 1.0, 300.0);
	CHECK(thermo != nullptr, "Thermodynamics handle creation");
	CHECK(approx(td_get_temperature(thermo), 300.0), "Thermodynamics temperature");
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
		!test_module_apis() ||
		!test_coupling_api() ||
		!test_async_api()) {
		return 1;
	}

	std::cout << "PhysicsEngine integration tests passed.\n";
	return 0;
}
