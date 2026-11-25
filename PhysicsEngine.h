#pragma once

// ╔══════════════════════════════════════════════════════════════════════════════╗
//  PhysicsEngine.h — Master include header for the unified physics engine
//
//  Single header that exposes all 8 submodule C APIs plus the async task
//  system and cross-module coupling layer. Module-specific ownership and
//  validation details are documented in API_CONTRACT.md.
//
//  Usage:
//    #include "PhysicsEngine.h"
//
//    QP_Handle qp = qp_create("electron", 9.109e-31, 1e-9, 1);
//    CP_Handle cp = cp_create("ball", 1.0, 1.0, 3);
//    PE_TaskId tid = pe_submit_async(myFunc, myArg);
//    ...
// ╚══════════════════════════════════════════════════════════════════════════════╝

#include "PhysicsMetadata.h"

// ── Individual module C APIs ────────────────────────────────────────────────
#include "quantum_c_api.h"           // QuantumCore          — QP_*
#include "classical_c_api.h"         // ClassicalMechanicsCore — CP_*
#include "em_c_api.h"                // ElectrodynamicsCore  — EM_*
#include "fluid_c_api.h"             // FluidDynamicsCore    — fluid_*
#include "nuclear_c_api.h"          // NuclearCore          — NP_*
#include "optics_c_api.h"           // OpticsCore           — OP_*
#include "solidstate_c_api.h"       // SolidStateCore       — ss_* (stateless)
#include "thermo_c_api.h"           // ThermodynamicsCore   — td_*

// ── Unified engine layers ───────────────────────────────────────────────────
#include "PhysicsAsync.h"            // Async task submission / polling
#include "PhysicsCoupling.h"         // Cross-module bridge functions
