#include "quantum_c_api.h"
#include <math.h>
#include <stdio.h>

static int callback_count;
static int invalid_callback_argument;
static double expected_parameter;
static double minimum_x;
static double maximum_x;

static double constant_potential(double x, double parameter)
{
	++callback_count;
	if (parameter != expected_parameter || !isfinite(x) || x < minimum_x - 1.0e-20 || x > maximum_x + 1.0e-20) {
		invalid_callback_argument = 1;
	}
	return parameter;
}

static int approximately_equal(double actual, double expected)
{
	return isfinite(actual) && fabs(actual - expected) <= 1.0e-9 * fabs(expected);
}

int main(void)
{
	const double mass = 9.1093837015e-31;
	const double hbar = 1.0545718e-34;
	const double pi = 3.14159265358979323846;
	const double width = 1.0e-10;
	const double height = 2.0e-19;
	const double energy = 1.0e-19;
	const double length = 1.0e-9;
	const double offset = 1.0e-20;
	QP_Handle particle = qp_create("callback-contract", mass, width, 1);
	if (!particle) {
		fputs("Failed to create Quantum handle.\n", stderr);
		return 1;
	}

	expected_parameter = height;
	minimum_x = -width;
	maximum_x = width;
	const double probability = qp_wkb_tunneling_probability(particle, constant_potential, height,
		minimum_x, maximum_x, energy, 32);
	const double expected_probability = exp(-4.0 * width * sqrt(2.0 * mass * (height - energy)) / hbar);
	if (callback_count == 0 || invalid_callback_argument || !approximately_equal(probability, expected_probability)) {
		fputs("Quantum tunneling callback check failed.\n", stderr);
		qp_destroy(particle);
		return 1;
	}

	callback_count = 0;
	invalid_callback_argument = 0;
	expected_parameter = offset;
	minimum_x = 0.0;
	maximum_x = length;
	const double result = qp_wkb_bohr_sommerfeld(particle, constant_potential, offset,
		minimum_x, maximum_x, 1, 32);
	const double target = 1.5 * pi * hbar;
	const double expected_result = offset + target * target / (2.0 * mass * length * length);
	if (callback_count == 0 || invalid_callback_argument || !approximately_equal(result, expected_result)) {
		fputs("Quantum Bohr-Sommerfeld callback check failed.\n", stderr);
		qp_destroy(particle);
		return 1;
	}

	qp_destroy(particle);
	return 0;
}
