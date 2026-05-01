#pragma once

// ╔══════════════════════════════════════════════════════════════════════════════╗
//  PhysicsEngine.h — Master include header for the unified physics engine
//
//  Single header that exposes all 8 submodule C APIs plus the async task
//  system and cross-module coupling layer.
//
//  Usage:
//    #include "PhysicsEngine.h"
//
//    QP_Handle qp = qp_create("electron", 9.109e-31, 1e-9, 1);
//    CP_Handle cp = cp_create("ball", 1.0, 1.0, 3);
//    PE_TaskId tid = pe_submit_async(myFunc, myArg);
//    ...
// ╚══════════════════════════════════════════════════════════════════════════════╝

#include "PhysicsEngine_export.h"

// ── Individual module C APIs ────────────────────────────────────────────────
#include "quantum_c_api.h"           // QuantumCore          — QP_*
#include "classical_c_api.h"         // ClassicalMechanicsCore — CP_*
#include "em_c_api.h"                // ElectrodynamicsCore  — EM_*
#include "fluid_c_api.h"            // FluidDynamicsCore    — fluid_*
#include "nuclear_c_api.h"          // NuclearCore          — NP_*
#include "optics_c_api.h"           // OpticsCore           — OP_*
#include "solidstate_c_api.h"       // SolidStateCore       — SS_*
#include "thermo_c_api.h"           // ThermodynamicsCore   — TH_*

// ── Unified engine layers ───────────────────────────────────────────────────
#include "PhysicsAsync.h"            // Async task submission / polling
#include "PhysicsCoupling.h"         // Cross-module bridge functions

#ifdef __cplusplus
extern "C" {
#endif

// ── Engine metadata ─────────────────────────────────────────────────────────

// Returns the engine version string (e.g. "1.0.0")
PHYSENG_API const char* pe_version(void);

// Returns the number of submodules linked into this engine
PHYSENG_API int pe_module_count(void);

// Returns the name of the i-th submodule (0-based)
// Returns NULL if index is out of range.
PHYSENG_API const char* pe_module_name(int index);

// Returns the number of simulation options in the i-th submodule
PHYSENG_API int pe_module_sim_count(int index);

#ifdef __cplusplus
}
#endif
