#include "fluid_c_api.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

bool approx(double actual, double expected, double relativeTolerance = 1.0e-12)
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

bool test_reference_values()
{
	constexpr double rho = 998.0;
	constexpr double g = 9.80665;
	constexpr double h = 12.0;
	constexpr double p0 = 101325.0;
	CHECK(approx(fluid_hydrostatic_pressure(rho, g, h, p0), p0 + rho * g * h),
		  "hydrostatic pressure reference");

	constexpr double p1 = 180000.0;
	constexpr double p2 = 100000.0;
	constexpr double h1 = 20.0;
	constexpr double h2 = 5.0;
	const double pressureHead = p1 - p2 + rho * g * (h1 - h2);
	CHECK(approx(fluid_bernoulli_velocity(p1, p2, rho, h1, h2, g),
				 std::sqrt(2.0 * pressureHead / rho)),
		  "Bernoulli velocity reference");

	constexpr double velocity = 1.5;
	constexpr double length = 0.05;
	constexpr double viscosity = 1.002e-3;
	CHECK(approx(fluid_reynolds_number(rho, velocity, length, viscosity),
				 rho * velocity * length / viscosity),
		  "Reynolds number reference");

	constexpr double gamma = 1.4;
	constexpr double gasConstant = 287.05;
	constexpr double temperature = 300.0;
	CHECK(approx(fluid_speed_of_sound(gamma, gasConstant, temperature),
				 std::sqrt(gamma * gasConstant * temperature)),
		  "speed of sound reference");

	constexpr double flowSpeed = 102.0;
	constexpr double soundSpeed = 340.0;
	CHECK(approx(fluid_mach_number(flowSpeed, soundSpeed), flowSpeed / soundSpeed),
		  "Mach number reference");
	return true;
}

bool test_observed_domain_behavior()
{
	CHECK(fluid_bernoulli_velocity(100000.0, 110000.0, 1000.0, 0.0, 0.0, 9.80665) == 0.0,
		  "negative Bernoulli pressure head returns zero");
	CHECK(fluid_hydrostatic_pressure(1000.0, 9.80665, 0.0, 101325.0) == 101325.0,
		  "zero-depth hydrostatic pressure");
	CHECK(fluid_mach_number(0.0, 340.0) == 0.0, "zero-speed Mach number");
	CHECK(fluid_hydrostatic_pressure(1000.0, 9.80665, -2.0, 101325.0) < 101325.0,
		  "negative depth reduces hydrostatic pressure");
	CHECK(approx(fluid_reynolds_number(1.0, 0.0, 1.0, 1.0), 0.0),
		  "zero velocity Reynolds number");
	CHECK(approx(fluid_speed_of_sound(1.4, 287.05, 0.0), 0.0),
		  "zero temperature speed of sound");
	return true;
}

} // namespace

int main()
{
	if (!test_reference_values() || !test_observed_domain_behavior()) {
		return 1;
	}

	std::cout << "Fluid API tests passed.\n";
	return 0;
}
