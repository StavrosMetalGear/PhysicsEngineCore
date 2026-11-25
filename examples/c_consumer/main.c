#include <PhysicsEngine.h>
#include <stdio.h>

int metadata_header_check(void);

int main(void)
{
	const char* version = pe_version();
	if (!metadata_header_check() || version == NULL || pe_module_count() != 8 ||
		(pe_capabilities() & (PE_CAPABILITY_FLUID | PE_CAPABILITY_QUANTUM)) !=
			(PE_CAPABILITY_FLUID | PE_CAPABILITY_QUANTUM) ||
		fluid_reynolds_number(1.0, 1.0, 1.0, 1.0) != 1.0) {
		return 1;
	}

	QP_Handle particle = qp_create("consumer", 2.0, 1.0, 1);
	if (particle == NULL) {
		return 1;
	}
	const double mass = qp_get_mass(particle);
	qp_destroy(particle);
	if (mass != 2.0) {
		return 1;
	}

	printf("%s C consumer OK\n", version);
	return 0;
}
