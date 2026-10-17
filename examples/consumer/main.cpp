#include <cmath>
#include <iostream>

#include <PhysicsEngine.h>
#include <physics/Sphere.h>

int main()
{
	if (pe_module_count() != 8) {
		return 1;
	}

	const double energy = pe_fermi_energy(8.5e28, 9.1093837015e-31);
	if (!std::isfinite(energy) || energy <= 0.0) {
		return 1;
	}

	Sphere sphere{{0.0f, 0.0f, 0.0f}, 1.0f};
	if (sphere.radius != 1.0) {
		return 1;
	}

	std::cout << pe_version() << " consumer OK\n";
	return 0;
}