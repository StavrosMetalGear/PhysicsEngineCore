#pragma once

#include "PhysicsEngine_export.h"
#include <stdint.h>

// Current C API revision; compatibility guarantees are being defined in Phase 11.
#define PE_API_VERSION_MAJOR 1
#define PE_API_VERSION_MINOR 0
#define PE_API_VERSION_PATCH 0

// Capability bits describe APIs compiled into this library, not runtime readiness.
#define PE_CAPABILITY_METADATA        (UINT64_C(1) << 0)
#define PE_CAPABILITY_ASYNC           (UINT64_C(1) << 1)
#define PE_CAPABILITY_COUPLING        (UINT64_C(1) << 2)
#define PE_CAPABILITY_QUANTUM         (UINT64_C(1) << 3)
#define PE_CAPABILITY_CLASSICAL       (UINT64_C(1) << 4)
#define PE_CAPABILITY_ELECTRODYNAMICS  (UINT64_C(1) << 5)
#define PE_CAPABILITY_FLUID           (UINT64_C(1) << 6)
#define PE_CAPABILITY_NUCLEAR         (UINT64_C(1) << 7)
#define PE_CAPABILITY_OPTICS          (UINT64_C(1) << 8)
#define PE_CAPABILITY_SOLID_STATE     (UINT64_C(1) << 9)
#define PE_CAPABILITY_THERMODYNAMICS  (UINT64_C(1) << 10)

#ifdef __cplusplus
extern "C" {
#endif

// Returns the engine version string (e.g. "1.0.0"). The returned pointer is
// owned by the engine and remains valid for the process lifetime.
PHYSENG_API const char* pe_version(void);

// Returns a bitmask of compiled-in API capabilities. Ignore unknown bits.
PHYSENG_API uint64_t pe_capabilities(void);

// Returns the number of submodules linked into this engine.
PHYSENG_API int pe_module_count(void);

// Returns the name of the i-th submodule (0-based)
// Returns NULL if index is out of range. The returned pointer is owned by the
// engine and remains valid for the process lifetime.
PHYSENG_API const char* pe_module_name(int index);

// Returns the number of simulation options in the i-th submodule, or 0 for an
// invalid index.
PHYSENG_API int pe_module_sim_count(int index);

#ifdef __cplusplus
}
#endif
