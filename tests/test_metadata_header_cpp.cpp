#include "PhysicsMetadata.h"
#include <type_traits>

static_assert(std::is_same_v<decltype(&pe_version), const char* (*)()>);
static_assert(std::is_same_v<decltype(&pe_capabilities), uint64_t (*)()>);
static_assert(std::is_same_v<decltype(&pe_module_count), int (*)()>);
static_assert(std::is_same_v<decltype(&pe_module_name), const char* (*)(int)>);
static_assert(std::is_same_v<decltype(&pe_module_sim_count), int (*)(int)>);
static_assert(PE_API_VERSION_MAJOR == 1);
static_assert(PE_CAPABILITY_METADATA == UINT64_C(1));
