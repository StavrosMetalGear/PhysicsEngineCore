#include "experimental/SimulationWorld.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace physics::experimental;

#define CHECK(condition, message) \
	do { if (!(condition)) { std::cerr << "FAIL: " << message << '\n'; return 1; } } while (false)

static bool near(float actual, float expected)
{
	return std::fabs(actual - expected) < 1.0e-5f;
}

int main()
{
	bool rejected = false;
	try { SimulationWorld invalid(1, 0.0f, {0.0f, -10.0f, 0.0f}); }
	catch (const std::invalid_argument&) { rejected = true; }
	CHECK(rejected, "zero timestep rejected");
	rejected = false;
	try { SimulationWorld invalid(1, 0.1f, {0.0f, std::numeric_limits<float>::infinity(), 0.0f}); }
	catch (const std::invalid_argument&) { rejected = true; }
	CHECK(rejected, "non-finite gravity rejected");

	SimulationWorld world(2, 0.1f, {0.0f, -10.0f, 0.0f});
	const SphereBody initial{{0.0f, 2.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.5f};
	const BodyId first = world.addSphere(initial);
	CHECK(first != invalidBodyId && world.bodyCount() == 1, "body added");
	SphereBody invalid = initial;
	invalid.radius = 0.0f;
	CHECK(world.addSphere(invalid) == invalidBodyId && world.bodyCount() == 1, "invalid radius rejected");
	invalid = initial;
	invalid.position[0] = std::numeric_limits<float>::quiet_NaN();
	CHECK(world.addSphere(invalid) == invalidBodyId, "non-finite position rejected");
	const BodyId second = world.addSphere(initial);
	CHECK(second != invalidBodyId && second != first, "distinct identifiers");
	CHECK(world.addSphere(initial) == invalidBodyId && world.bodyCount() == 2, "capacity enforced");
	SphereBody observed{};
	CHECK(!world.getSphere(invalidBodyId, observed), "unknown identifier rejected");
	CHECK(world.step() && world.getSphere(first, observed), "first step");
	CHECK(near(observed.position[0], 0.1f) && near(observed.position[1], 1.9f) &&
		near(observed.velocity[1], -1.0f), "semi-implicit Euler gravity");
	for (int i = 0; i < 9; ++i) CHECK(world.step(), "falling step");
	CHECK(world.getSphere(first, observed), "first body accessible");
	CHECK(near(observed.position[1], 0.5f) && near(observed.velocity[1], 0.0f), "floor contact");
	SphereBody observedSecond{};
	CHECK(world.getSphere(second, observedSecond), "second body accessible");
	CHECK(observed.position == observedSecond.position && observed.velocity == observedSecond.velocity,
		"equal inputs follow the same steps");

	CHECK(!world.removeSphere(invalidBodyId) && world.bodyCount() == 2, "invalid removal unchanged");
	CHECK(world.removeSphere(first) && world.bodyCount() == 1, "remove first body");
	const SphereBody sentinel{{42.0f, 43.0f, 44.0f}, {1.0f, 2.0f, 3.0f}, 2.0f};
	observed = sentinel;
	CHECK(!world.getSphere(first, observed) && observed.position == sentinel.position &&
		observed.velocity == sentinel.velocity && observed.radius == sentinel.radius, "removed ID leaves output unchanged");
	CHECK(!world.removeSphere(first) && world.bodyCount() == 1, "repeated removal unchanged");
	const BodyId replacement = world.addSphere(initial);
	CHECK(replacement > second && world.bodyCount() == 2, "capacity reused with a new identifier");
	CHECK(world.getSphere(second, observed) && observed.position == observedSecond.position &&
		observed.velocity == observedSecond.velocity, "surviving identifier and state preserved");
	CHECK(world.step(), "step after removal and replacement");
	CHECK(world.getSphere(second, observed) && near(observed.position[1], 0.5f), "survivor remains on floor");
	CHECK(world.getSphere(replacement, observed) && near(observed.position[1], 1.9f), "replacement advances");
	CHECK(world.removeSphere(replacement) && world.bodyCount() == 1, "remove last body");
	CHECK(world.removeSphere(second) && world.bodyCount() == 0 && world.step(), "remove final body and step empty world");
	const BodyId afterEmpty = world.addSphere(initial);
	CHECK(afterEmpty > replacement && !world.getSphere(first, observed) && !world.getSphere(second, observed),
		"identifiers remain stale after the world is emptied");

	SimulationWorld middle(3, 0.1f, {0.0f, 0.0f, 0.0f});
	const BodyId a = middle.addSphere(initial);
	const BodyId b = middle.addSphere(initial);
	const BodyId c = middle.addSphere(initial);
	CHECK(middle.removeSphere(b) && middle.bodyCount() == 2, "remove middle body");
	CHECK(middle.getSphere(a, observed) && middle.getSphere(c, observedSecond), "middle removal preserves both neighbors");
	CHECK(middle.step() && middle.getSphere(c, observedSecond) && near(observedSecond.position[0], 0.1f),
		"body after removed middle entry still advances");

	SimulationWorld overflow(2, 2.0f, {0.0f, 0.0f, 0.0f});
	const BodyId safe = overflow.addSphere(initial);
	const SphereBody extreme{{0.0f, 2.0f, 0.0f}, {std::numeric_limits<float>::max(), 0.0f, 0.0f}, 0.5f};
	CHECK(overflow.addSphere(extreme) != invalidBodyId, "finite extreme input accepted");
	CHECK(!overflow.step(), "overflow fails the step");
	CHECK(overflow.getSphere(safe, observed) && observed.position == initial.position && observed.velocity == initial.velocity,
		"failed step leaves earlier bodies unchanged");
	std::cout << "Simulation world checks passed.\n";
	return 0;
}
