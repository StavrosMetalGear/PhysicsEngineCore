#pragma once

#include <stdint.h>

// Snapshot of engine-owned declarations at d813a60; this is not a released ABI baseline.
#define PE_ENGINE_SIGNATURE_BASELINE(X) \
	X(pe_version, const char*, (void)) \
	X(pe_capabilities, uint64_t, (void)) \
	X(pe_module_count, int, (void)) \
	X(pe_module_name, const char*, (int)) \
	X(pe_module_sim_count, int, (int)) \
	X(pe_async_init, void, (int)) \
	X(pe_async_shutdown, void, (void)) \
	X(pe_submit_async, uint64_t, (void (*)(void*), void*)) \
	X(pe_task_ready, int, (uint64_t)) \
	X(pe_task_wait, void, (uint64_t)) \
	X(pe_tasks_pending, int, (void)) \
	X(pe_nusselt_flat_plate, double, (double, double, double, double, double, double)) \
	X(pe_convective_heat_flux, double, (double, double, double, double, double)) \
	X(pe_skin_depth, double, (double, double)) \
	X(pe_drude_reflectance, double, (double, double, double)) \
	X(pe_quantum_dot_confinement, double, (double, double)) \
	X(pe_fermi_energy, double, (double, double)) \
	X(pe_gamow_peak, double, (double, int, int, double)) \
	X(pe_terminal_velocity, double, (double, double, double, double)) \
	X(pe_fission_thermal_power, double, (double, double))
