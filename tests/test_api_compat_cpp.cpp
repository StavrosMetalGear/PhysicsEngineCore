#include "PhysicsEngine.h"
#include <cstdint>
#include <type_traits>

static_assert(std::is_same_v<decltype(&pe_version), const char* (*)()>);
static_assert(std::is_same_v<decltype(&pe_module_count), int (*)()>);
static_assert(std::is_same_v<PE_TaskId, uint64_t>);
static_assert(std::is_same_v<PE_TaskFunc, void (*)(void*)>);
static_assert(std::is_same_v<decltype(&pe_submit_async), PE_TaskId (*)(PE_TaskFunc, void*)>);
static_assert(std::is_same_v<decltype(&pe_fermi_energy), double (*)(double, double)>);

static_assert(std::is_same_v<decltype(&qp_create), QP_Handle (*)(const char*, double, double, int)>);
static_assert(std::is_same_v<decltype(&cp_create), CP_Handle (*)(const char*, double, double, int)>);
static_assert(std::is_same_v<decltype(&em_create), EM_Handle (*)(const char*, double, double, double)>);
static_assert(std::is_same_v<decltype(&fluid_reynolds_number), double (*)(double, double, double, double)>);
static_assert(std::is_same_v<decltype(&np_create), NP_Handle (*)()>);
static_assert(std::is_same_v<decltype(&op_create), OP_Handle (*)(const char*, double, double)>);
static_assert(std::is_same_v<decltype(&ss_lattice_spacing), double (*)(int, int, int, double)>);
static_assert(std::is_same_v<decltype(&td_create), TD_Handle (*)(const char*, double, double)>);
