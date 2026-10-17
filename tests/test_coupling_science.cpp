#include "PhysicsCoupling.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace {

bool approx(double actual, double expected, double relative = 1.0e-10,
			double absolute = 1.0e-30)
{
	return std::abs(actual - expected) <=
		   std::max(absolute, relative * std::abs(expected));
}

bool finite_nan(double value)
{
	return std::isnan(value);
}

bool check(bool condition, const char* name)
{
	if (!condition) {
		std::cerr << "FAILED: " << name << '\n';
		return false;
	}
	return true;
}

bool test_reference_values()
{
	constexpr double pi = 3.14159265358979323846;
	constexpr double hbar = 1.0545718e-34;
	constexpr double mu0 = 1.2566370614e-6;
	constexpr double kb = 1.380649e-23;
	constexpr double alpha = 1.0 / 137.035999084;
	constexpr double c = 2.99792458e8;
	constexpr double g = 9.80665;
	constexpr double electronCharge = 1.602176634e-19;

	const double rho = 998.0;
	const double velocity = 1.5;
	const double length = 0.05;
	const double viscosity = 1.002e-3;
	const double heatCapacity = 4181.0;
	const double conductivity = 0.6;
	const double reynolds = rho * velocity * length / viscosity;
	const double prandtl = heatCapacity * viscosity / conductivity;
	const double expectedNu = 0.332 * std::sqrt(reynolds) * std::cbrt(prandtl);
	if (!check(approx(pe_nusselt_flat_plate(rho, velocity, length, viscosity,
											 heatCapacity, conductivity), expectedNu),
			   "Nusselt reference")) return false;

	if (!check(approx(pe_convective_heat_flux(10.0, 0.5, 0.1, 300.0, 280.0),
					  1000.0), "heat-flux reference")) return false;

	if (!check(approx(pe_skin_depth(5.8e7, 1.0e9),
					  std::sqrt(2.0 / (mu0 * 5.8e7 * 1.0e9))),
			   "skin-depth reference")) return false;

	const double omegaP = 1.5e16;
	const double gamma = 1.0e14;
	const double omega = 3.0e15;
	const double w2 = omega * omega;
	const double denom = w2 + gamma * gamma;
	const double epsR = 1.0 - (omegaP * omegaP * w2) /
								  (w2 * denom + 1.0e-300);
	const double epsI = (omegaP * omegaP * gamma * omega) /
						(w2 * denom + 1.0e-300);
	const double absEps = std::sqrt(epsR * epsR + epsI * epsI);
	const double n = std::sqrt((absEps + epsR) / 2.0);
	const double k = std::sqrt((absEps - epsR) / 2.0);
	const double expectedReflectance = ((n - 1.0) * (n - 1.0) + k * k) /
									   ((n + 1.0) * (n + 1.0) + k * k + 1.0e-300);
	if (!check(approx(pe_drude_reflectance(omegaP, gamma, omega), expectedReflectance),
			   "Drude reflectance reference")) return false;

	const double mass = 0.067 * 9.1093837015e-31;
	const double dotSize = 5.0e-9;
	const double expectedConfinement = (pi * pi * hbar * hbar) /
									   (2.0 * mass * dotSize * dotSize);
	if (!check(approx(pe_quantum_dot_confinement(mass, dotSize), expectedConfinement),
			   "quantum confinement reference")) return false;

	const double density = 8.5e28;
	const double electronMass = 9.1093837015e-31;
	const double expectedFermi = (hbar * hbar / (2.0 * electronMass)) *
								 std::pow(3.0 * pi * pi * density, 2.0 / 3.0);
	if (!check(approx(pe_fermi_energy(density, electronMass), expectedFermi),
			   "Fermi energy reference")) return false;

	const double temperature = 1.5e7;
	const int z1 = 1;
	const int z2 = 1;
	const double reducedMass = 0.5 * 1.67262192369e-27;
	const double b = pi * alpha * z1 * z2 *
					 std::sqrt(2.0 * reducedMass * c * c);
	const double expectedGamow = std::pow(b * kb * temperature / 2.0, 2.0 / 3.0);
	if (!check(approx(pe_gamow_peak(temperature, z1, z2, reducedMass), expectedGamow),
			   "Gamow peak reference")) return false;

	const double expectedVelocity = (2.0 / 9.0) * 999.0 * g * 0.01 * 0.01 / 1.0e-3;
	if (!check(approx(pe_terminal_velocity(1000.0, 1.0, 0.01, 1.0e-3),
					  expectedVelocity), "terminal velocity reference")) return false;

	if (!check(approx(pe_fission_thermal_power(1.0e12, 200.0),
					  1.0e12 * 200.0 * 1.0e6 * electronCharge),
			   "fission power reference")) return false;

	return true;
}

bool test_limits_and_domains()
{
	const double nan = std::numeric_limits<double>::quiet_NaN();
	const double inf = std::numeric_limits<double>::infinity();

	if (!check(pe_nusselt_flat_plate(998.0, 0.0, 0.05, 1.0e-3, 4181.0, 0.6) == 0.0,
			   "zero velocity limit")) return false;
	if (!check(pe_fermi_energy(0.0, 9.109e-31) == 0.0, "zero density limit")) return false;
	if (!check(pe_gamow_peak(0.0, 1, 1, 1.0e-27) == 0.0, "zero temperature limit")) return false;
	if (!check(pe_terminal_velocity(1000.0, 1.0, 0.0, 1.0e-3) == 0.0,
			   "zero radius limit")) return false;
	if (!check(pe_fission_thermal_power(0.0, 200.0) == 0.0, "zero rate limit")) return false;

	if (!check(finite_nan(pe_nusselt_flat_plate(1.0, 1.0, 1.0, 0.0, 1.0, 1.0)),
			   "Nusselt zero viscosity rejection")) return false;
	if (!check(finite_nan(pe_convective_heat_flux(1.0, 1.0, 0.0, 1.0, 0.0)),
			   "heat-flux zero length rejection")) return false;
	if (!check(finite_nan(pe_skin_depth(0.0, 1.0)), "skin-depth zero conductivity rejection")) return false;
	if (!check(finite_nan(pe_drude_reflectance(1.0, 1.0, 0.0)),
			   "Drude zero frequency rejection")) return false;
	if (!check(finite_nan(pe_quantum_dot_confinement(-1.0, 1.0)),
			   "confinement negative mass rejection")) return false;
	if (!check(finite_nan(pe_fermi_energy(1.0, 0.0)), "Fermi zero mass rejection")) return false;
	if (!check(finite_nan(pe_gamow_peak(1.0, 0, 1, 1.0)), "Gamow invalid charge rejection")) return false;
	if (!check(finite_nan(pe_terminal_velocity(1.0, 1.0, 1.0, 0.0)),
			   "terminal velocity zero viscosity rejection")) return false;
	if (!check(finite_nan(pe_fission_thermal_power(-1.0, 200.0)),
			   "fission negative rate rejection")) return false;

	if (!check(finite_nan(pe_skin_depth(nan, 1.0)) &&
			   finite_nan(pe_skin_depth(inf, 1.0)), "nonfinite input rejection")) return false;

	return true;
}

bool test_physical_trends()
{
	if (!check(pe_quantum_dot_confinement(1.0e-30, 1.0e-9) >
			   pe_quantum_dot_confinement(1.0e-30, 2.0e-9),
			   "confinement decreases with size")) return false;
	if (!check(pe_fermi_energy(8.0e28, 1.0e-30) >
			   pe_fermi_energy(1.0e28, 1.0e-30),
			   "Fermi energy increases with density")) return false;
	if (!check(pe_skin_depth(1.0e6, 1.0e9) > pe_skin_depth(1.0e6, 2.0e9),
			   "skin depth decreases with frequency")) return false;
	if (!check(pe_gamow_peak(2.0e7, 1, 1, 1.0e-27) >
			   pe_gamow_peak(1.0e7, 1, 1, 1.0e-27),
			   "Gamow peak increases with temperature")) return false;
	if (!check(pe_drude_reflectance(1.0e16, 1.0e14, 1.0e12) > 0.9,
			   "Drude low-frequency reflectance")) return false;
	if (!check(pe_drude_reflectance(1.0e16, 1.0e14, 1.0e18) < 0.01,
			   "Drude high-frequency reflectance")) return false;
	return true;
}

} // namespace

int main()
{
	if (!test_reference_values() || !test_limits_and_domains() || !test_physical_trends()) {
		return 1;
	}

	std::cout << "Phase 6 coupling science tests passed\n";
	return 0;
}
