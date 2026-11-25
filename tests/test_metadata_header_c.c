#include "PhysicsMetadata.h"

_Static_assert(_Generic(&pe_version, const char* (*)(void): 1, default: 0), "pe_version signature");
_Static_assert(_Generic(&pe_capabilities, uint64_t (*)(void): 1, default: 0), "pe_capabilities signature");
_Static_assert(_Generic(&pe_module_count, int (*)(void): 1, default: 0), "pe_module_count signature");
_Static_assert(_Generic(&pe_module_name, const char* (*)(int): 1, default: 0), "pe_module_name signature");
_Static_assert(_Generic(&pe_module_sim_count, int (*)(int): 1, default: 0), "pe_module_sim_count signature");
_Static_assert(PE_API_VERSION_MAJOR == 1, "API version");
_Static_assert(PE_CAPABILITY_METADATA == UINT64_C(1), "metadata capability");
