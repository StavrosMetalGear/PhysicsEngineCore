#include "PhysicsEngine.h"
#include <stddef.h>
#include <stdint.h>

#define CHECK_SIGNATURE(function, type) \
	_Static_assert(_Generic(&(function), type: 1, default: 0), #function " signature changed")

CHECK_SIGNATURE(pe_version, const char* (*)(void));
CHECK_SIGNATURE(pe_module_count, int (*)(void));
CHECK_SIGNATURE(pe_submit_async, PE_TaskId (*)(PE_TaskFunc, void*));
CHECK_SIGNATURE(pe_fermi_energy, double (*)(double, double));
CHECK_SIGNATURE(qp_create, QP_Handle (*)(const char*, double, double, int));
CHECK_SIGNATURE(cp_create, CP_Handle (*)(const char*, double, double, int));
CHECK_SIGNATURE(em_create, EM_Handle (*)(const char*, double, double, double));
CHECK_SIGNATURE(fluid_reynolds_number, double (*)(double, double, double, double));
CHECK_SIGNATURE(np_create, NP_Handle (*)(void));
CHECK_SIGNATURE(op_create, OP_Handle (*)(const char*, double, double));
CHECK_SIGNATURE(ss_lattice_spacing, double (*)(int, int, int, double));
CHECK_SIGNATURE(td_create, TD_Handle (*)(const char*, double, double));
_Static_assert(_Generic((PE_TaskId)0, uint64_t: 1, default: 0), "PE_TaskId changed");
_Static_assert(_Generic((PE_TaskFunc)0, void (*)(void*): 1, default: 0), "PE_TaskFunc changed");

static QP_Handle (*volatile quantum_create)(const char*, double, double, int) = qp_create;
static CP_Handle (*volatile classical_create)(const char*, double, double, int) = cp_create;
static EM_Handle (*volatile em_create_fn)(const char*, double, double, double) = em_create;
static double (*volatile fluid_reynolds)(double, double, double, double) = fluid_reynolds_number;
static NP_Handle (*volatile nuclear_create)(void) = np_create;
static OP_Handle (*volatile optics_create)(const char*, double, double) = op_create;
static double (*volatile lattice_spacing)(int, int, int, double) = ss_lattice_spacing;
static TD_Handle (*volatile thermal_create)(const char*, double, double) = td_create;
static PE_TaskId (*volatile submit_async)(PE_TaskFunc, void*) = pe_submit_async;
static double (*volatile coupling_fermi_energy)(double, double) = pe_fermi_energy;

int main(void)
{
	if (!quantum_create || !classical_create || !em_create_fn || !fluid_reynolds ||
		!nuclear_create || !optics_create || !lattice_spacing || !thermal_create ||
		!submit_async || !coupling_fermi_energy) {
		return 1;
	}
	if (pe_version() == NULL || pe_module_count() != 8 ||
		fluid_reynolds(1.0, 1.0, 1.0, 1.0) != 1.0) {
		return 1;
	}
	return 0;
}
