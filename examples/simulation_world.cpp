#include "experimental/SimulationWorld.h"

#include <iostream>

int main()
{
	using namespace physics::experimental;
	SimulationWorld world(1, 1.0f / 60.0f, {0.0f, -9.81f, 0.0f});
	const BodyId ball = world.addSphere({{0.0f, 3.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.5f});
	if (ball == invalidBodyId) return 1;
	for (int tick = 0; tick < 120; ++tick) {
		if (!world.step()) return 1;
	}
	SphereBody body{};
	if (!world.getSphere(ball, body)) return 1;
	std::cout << "After 120 ticks: height=" << body.position[1] << " m, vertical speed="
			  << body.velocity[1] << " m/s\n";
	return 0;
}
