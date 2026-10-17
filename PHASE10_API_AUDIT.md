# Phase 10 Module API Audit

## Scope

This audit compares the eight pinned sibling repositories listed in
`dependencies/PhysicsCoreRepos.cmake` with the sources aggregated by
`CMakeLists.txt`. The pinned revision is the CI source of truth.

## Module mapping

| Module | Pinned revision | C API implementation | C API declaration | Aggregated implementation sources | Unified public header |
|---|---|---|---|---|---|
| QuantumCore | `529d5912af6f2e15f1f1a2b04b82f029b6c4e90c` | `quantum_c_api.cpp` | sibling `quantum_c_api.h` | `QuantumParticle.cpp`, `QuantumParticleCompatibility.cpp`, `NumericalSolver.cpp`, `quantum_c_api.cpp` | `PhysicsEngine.h` |
| ClassicalMechanicsCore | `044a8fa6f13fbb32b96b45a27d499fafd853d088` | `classical_c_api.cpp` | sibling `classical_c_api.h` | `ClassicalParticle.cpp`, `NumericalSolver.cpp`, `classical_c_api.cpp` | `PhysicsEngine.h` |
| ElectrodynamicsCore | `8ac6b1b0129f2815ff2c1f983eeab792307927b1` | `em_c_api.cpp` | sibling `em_c_api.h` | `EMField.cpp`, `NumericalSolverEM.cpp`, `em_c_api.cpp` | `PhysicsEngine.h` |
| FluidDynamicsCore | `895136a5d42271f4eaac840fece6987faefc6e7a` | `fluid_c_api.cpp` | local `fluid_c_api.h` | `FluidSystem.cpp`, `NumericalSolverFD.cpp`, `fluid_c_api.cpp` | `PhysicsEngine.h` |
| NuclearCore | `b3f7c180349d4b5d827ce27ba61bfbe0068f0497` | `nuclear_c_api.cpp` | sibling `nuclear_c_api.h` | `NuclearSystem.cpp`, `NumericalSolverNuc.cpp`, `nuclear_c_api.cpp` | `PhysicsEngine.h` |
| OpticsCore | `f400d442191c64a1985ce49ddeed60b0dce4643b` | `optics_c_api.cpp` | sibling `optics_c_api.h` | `OpticalSystem.cpp`, `NumericalSolverOpt.cpp`, `optics_c_api.cpp` | `PhysicsEngine.h` |
| SolidStateCore | `d9fffbbcbe7da6417119cc5e49fc02d1d25b1982` | `solidstate_c_api.cpp` | sibling `solidstate_c_api.h` | `SolidStateSystem.cpp`, `NumericalSolverSS.cpp`, `solidstate_c_api.cpp` | `PhysicsEngine.h` |
| ThermodynamicsCore | `c2013de01d5dd9b202b7ae9df7c4e10b973463a6` | `thermo_c_api.cpp` | sibling `thermo_c_api.h` | `ThermoSystem.cpp`, `NumericalSolverTD.cpp`, `thermo_c_api.cpp` | `PhysicsEngine.h` |

## Findings

1. All eight module C API implementation files are included in the aggregate
   `PhysicsEngine` static library through their object libraries.
2. Seven modules provide their C API declaration header in the pinned sibling
   repository. FluidDynamicsCore provides no `fluid_c_api.h` at the pinned
   revision.
3. The project-owned `fluid_c_api.h` is therefore the deliberate public
   declaration boundary for the five compiled Fluid wrappers:
   `fluid_hydrostatic_pressure`, `fluid_bernoulli_velocity`,
   `fluid_reynolds_number`, `fluid_speed_of_sound`, and `fluid_mach_number`.
4. QuantumCore's pinned C API declares two member functions absent from the
   pinned `QuantumParticle.cpp`. `QuantumParticleCompatibility.cpp` supplies
   those definitions in this repository, preserving the exposed wrappers
   without modifying the sibling repository.
5. The existing `PhysicsEngine.h` aggregates all eight module C APIs. The
   collision headers under `include/physics` are a separate C++17 API and are
   not part of the module C ABI.

## Phase 10 actionable gap

The first concrete coverage task is to add reference-value tests for the five
Fluid C API wrappers and verify that the installed package exposes the local
header correctly. Before expanding coverage to the larger stateful APIs, the
remaining module declarations should be inventoried with a parser-aware tool
because raw regular-expression counts are distorted by multiline signatures,
helper functions, and C++ implementation functions in the wrapper sources.
