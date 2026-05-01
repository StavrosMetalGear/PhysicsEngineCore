# PhysicsEngineCore

**Unified C++ physics engine aggregating 8 domain-specific submodules into a single static library with a flat C API.**

---

## Overview

PhysicsEngineCore links together all 8 physics *Core submodules — each containing 50 simulation options — into one cohesive library.  It provides:

| Layer | Description |
|-------|-------------|
| **Module C APIs** | `QP_*`, `CP_*`, `EM_*`, `FD_*`, `NP_*`, `OP_*`, `SS_*`, `TH_*` — opaque-handle C wrappers for each module |
| **Async Task System** | `pe_submit_async()`, `pe_task_ready()` — thread-pool for non-blocking computation |
| **Cross-Module Coupling** | `pe_nusselt_flat_plate()`, `pe_skin_depth()`, `pe_fermi_energy()`, etc. — bridge functions spanning multiple modules |
| **Engine Metadata** | `pe_version()`, `pe_module_count()`, `pe_module_name()` — runtime introspection |

## Submodules (400 Simulations Total)

| # | Module | Lib Target | C API Prefix | Simulations |
|---|--------|-----------|-------------|-------------|
| 1 | QuantumCore | `QuantumPhysics` | `qp_*` | Quantum mechanics (ISW, HO, hydrogen, scattering, entanglement, ...) |
| 2 | ClassicalMechanicsCore | `ClassicalPhysics` | `cp_*` | Newtonian mechanics, Lagrangian/Hamiltonian, rigid body, oscillations |
| 3 | ElectrodynamicsCore | `EMPhysics` | `em_*` | Maxwell's equations, EM waves, circuits, radiation |
| 4 | FluidDynamicsCore | `FluidDynamicsPhysics` | `fd_*` | Navier-Stokes, compressible flow, turbulence, MHD |
| 5 | NuclearCore | `NuclearPhysics` | `np_*` | Nuclear structure, decay, fission, fusion, dosimetry |
| 6 | OpticsCore | `OpticsPhysics` | `op_*` | Geometric optics, wave optics, polarization, fiber optics |
| 7 | SolidStateCore | `SolidStatePhysics` | `ss_*` | Crystal structure, band theory, semiconductors, magnetism |
| 8 | ThermodynamicsCore | `ThermoPhysics` | `th_*` | Laws of thermodynamics, statistical mechanics, phase transitions |

## Building

### Prerequisites
- CMake 3.15+
- C++17 compiler (MSVC 2022 / GCC / Clang)
- All 8 submodule repositories in the same parent directory (`../QuantumCore`, `../FluidDynamicsCore`, etc.)

### Build Steps
```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The build produces **`PhysicsEngine.lib`** (static library) in `build/lib/`.

## Directory Structure

```
PhysicsEngineCore/
├── CMakeLists.txt            # Master build — adds all 8 submodules
├── PhysicsEngine.h           # Single master include header
├── PhysicsEngine.cpp         # Engine metadata (version, module registry)
├── PhysicsEngine_export.h    # DLL/static export macros
├── PhysicsAsync.h/.cpp       # Thread-pool async task system
├── PhysicsCoupling.h/.cpp    # Cross-module bridge functions
├── fluid_c_api.h/.cpp        # C API for FluidDynamicsCore (was missing)
└── README.md
```

## Usage Example (C++)

```cpp
#include "PhysicsEngine.h"
#include <cstdio>

int main() {
    printf("PhysicsEngineCore v%s — %d modules\n",
           pe_version(), pe_module_count());

    // Quantum: electron in a 1D box
    QP_Handle qp = qp_create("electron", 9.109e-31, 1e-9, 1);
    printf("E_3 = %.6e J\n", qp_energy_1d_box(qp, 3));
    qp_destroy(qp);

    // Fluid: water pipe
    FD_Handle fd = fd_create("water", 998.0, 1.002e-3, 0.05, 1.5);
    printf("Re = %.1f\n", fd_reynoldsNumber(fd));
    fd_destroy(fd);

    // Async: run a heavy computation off the main thread
    pe_async_init(4);
    PE_TaskId tid = pe_submit_async([](void*) {
        // ... expensive physics computation ...
    }, nullptr);
    pe_task_wait(tid);
    pe_async_shutdown();

    // Cross-module: Fermi energy from density + effective mass
    double E_F = pe_fermi_energy(8.5e28, 9.109e-31);
    printf("E_F = %.4f eV\n", E_F / 1.602e-19);

    return 0;
}
```

## Unreal Engine Integration

This library is designed to be linked into an Unreal Engine plugin:

1. Build `PhysicsEngine.lib` (Release, static).
2. In your UE plugin's `.Build.cs`:
   ```csharp
   PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyDir, "PhysicsEngine.lib"));
   PublicIncludePaths.Add(Path.Combine(ThirdPartyDir, "include"));
   ```
3. Use `pe_submit_async()` from game threads to avoid blocking the render loop.
4. Create `BlueprintCallable` wrappers in your plugin's C++ code.

## Cross-Module Coupling Functions

| Function | Modules | Description |
|----------|---------|-------------|
| `pe_nusselt_flat_plate()` | Fluid + Thermo | Laminar flat-plate Nusselt number |
| `pe_convective_heat_flux()` | Fluid + Thermo | Heat transfer rate from Nu |
| `pe_skin_depth()` | EM + Optics | EM wave skin depth in conductor |
| `pe_drude_reflectance()` | EM + Optics | Metal reflectance from Drude model |
| `pe_quantum_dot_confinement()` | Quantum + SolidState | QD confinement energy |
| `pe_fermi_energy()` | Quantum + SolidState | Free electron Fermi energy |
| `pe_gamow_peak()` | Nuclear + Quantum | Stellar fusion Gamow peak |
| `pe_terminal_velocity()` | Classical + Fluid | Stokes drag terminal velocity |
| `pe_fission_thermal_power()` | Thermo + Nuclear | Reactor thermal power |

## License

Private repository — all rights reserved.

---

*Part of the PhysicsCore suite by Stavros.*
