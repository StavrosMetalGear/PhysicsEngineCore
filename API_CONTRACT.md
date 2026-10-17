# PhysicsEngineCore Public API Contract

## Scope

This document records the public contract observed in the current source tree. The
module headers in the sibling repositories remain the authoritative declaration
sources; this file explains how they are aggregated and what callers may currently
rely on.

The exported module APIs use C linkage and are intended to be callable from C and
C-compatible consumers. The collision subsystem under `include/physics` is C++17
and is not part of the C ABI.

## Module inventory

| Module | Authoritative header | Handle/type family | Function family | Current state |
|---|---|---|---|---|
| QuantumCore | `../QuantumCore/quantum_c_api.h` | `QP_Handle` | `qp_*` | Opaque handle plus state/vector and numerical helpers |
| ClassicalMechanicsCore | `../ClassicalMechanicsCore/classical_c_api.h` | `CP_Handle` | `cp_*` | Opaque particle/system handle plus mechanics helpers |
| ElectrodynamicsCore | `../ElectrodynamicsCore/em_c_api.h` | `EM_Handle` | `em_*` | Opaque electromagnetic-field handle plus field helpers |
| FluidDynamicsCore | `fluid_c_api.h` and `../FluidDynamicsCore/fluid_c_api.cpp` | No handle | `fluid_*` | Stateless wrapper functions currently exposed by this project |
| NuclearCore | `../NuclearCore/nuclear_c_api.h` | `NP_Handle` | `np_*` | Opaque nuclear-system handle plus nuclear helpers |
| OpticsCore | `../OpticsCore/optics_c_api.h` | `OP_Handle` | `op_*` | Opaque optical-system handle plus optics helpers |
| SolidStateCore | `../SolidStateCore/solidstate_c_api.h` | No handle | `ss_*` | Plain `extern "C"` stateless functions; no export macro currently |
| ThermodynamicsCore | `../ThermodynamicsCore/thermo_c_api.h` | `TD_Handle` | `td_*` | Opaque thermodynamics-system handle plus thermal/statistical helpers |

The complete exported function inventory is defined by the declarations in those
seven sibling headers plus the local fluid header. The headers are intentionally
kept as the single source of truth because several modules expose substantially
more than the representative functions shown below.

## Naming and linkage

The actual naming differs from the original README table:

- Quantum uses `QP_Handle` and `qp_*`.
- Classical mechanics uses `CP_Handle` and `cp_*`.
- Electrodynamics uses `EM_Handle` and `em_*`.
- Fluid currently uses `fluid_*`, not the README's `FD_*` notation.
- Nuclear uses `NP_Handle` and `np_*`.
- Optics uses `OP_Handle` and `op_*`.
- Solid state uses `ss_*` and does not currently use an opaque handle.
- Thermodynamics uses `TD_Handle` and `td_*`.

`PhysicsEngine.h` aggregates these APIs. The wrapper declarations use `extern "C"`
where supplied by the module headers. The `PHYSENG_API` macro controls this
repository's engine-level symbols for static and optional shared-library builds.
The individual sibling export macros are not completely uniform and must be
reviewed before shipping a shared library.

## Representative API groups

### Quantum

The Quantum API includes lifecycle operations such as `qp_create` and
`qp_destroy`, particle properties, one-dimensional box and oscillator calculations,
wavefunctions, matrices, spinors, density matrices, measurement, entanglement,
and numerical helpers. Quantum functions generally take a `QP_Handle` for the
stateful particle operations and raw arrays for mathematical operations.

### Classical mechanics

The Classical API includes `cp_create`, `cp_destroy`, getters/setters, free-particle
motion, projectile motion, oscillators, rigid-body operations, Lagrangian and
Hamiltonian helpers, N-body calculations, and CSV export functions.

### Electrodynamics

The EM API includes `em_create`, `em_destroy`, field/material properties, Maxwell
and wave helpers, circuits, radiation, boundary calculations, and numerical field
operations.

### Fluid

The current unified wrapper exposes these stateless functions:

- `fluid_hydrostatic_pressure`
- `fluid_bernoulli_velocity`
- `fluid_reynolds_number`
- `fluid_speed_of_sound`
- `fluid_mach_number`

These functions take ordinary numeric values and do not allocate or retain handles.
Additional FluidDynamicsCore functionality exists in the sibling repository but is
not automatically part of this wrapper unless it is declared and compiled here.

### Nuclear

The Nuclear API includes `np_create`, `np_destroy`, binding-energy calculations,
nuclear structure, decay, fission, fusion, and dosimetry helpers.

### Optics

The Optics API includes `op_create`, `op_destroy`, wavelength and medium properties,
geometrical optics, wave optics, polarization, interference, diffraction, and fiber
helpers.

### Solid state

The SolidState API currently exposes stateless functions for lattice spacing,
Bragg diffraction, dispersion, thermal properties, electronic properties,
semiconductor calculations, magnetism, superconductivity, and transport.

### Thermodynamics

The Thermodynamics API includes `td_create`, `td_destroy`, state properties, laws of
thermodynamics, phase transitions, statistical mechanics, transport, and quantum
statistical helpers.

## Ownership and lifetime

- A successful `*_create` call returns an opaque handle owned by the caller.
- The caller must pass that handle to the matching `*_destroy` function exactly once.
- Handles must not be copied into independently destroyed aliases.
- The implementation generally allocates handle objects with `new` and releases
  them with `delete`.
- Destroying an invalid, null, or already-destroyed handle is not documented as
  safe. Callers must keep ownership state and avoid double destruction.
- Stateless `fluid_*` and `ss_*` calls have no library-owned object lifetime.
- Pointer and array arguments remain caller-owned. The caller must keep input and
  output buffers valid for the duration of each call.
- String arguments passed to create functions are copied by the underlying systems
  where their implementations currently do so; callers should still treat all
  input pointers as required to remain valid during the call.

## Null and invalid-input behavior

The current wrappers do not provide a uniform error/status API.

- Most handle wrappers cast the opaque pointer and immediately dereference it.
  Null or invalid handles therefore have undefined behavior and must not be passed.
- Some create functions tolerate a null name by substituting an empty string; this
  is implementation-specific and is not a general contract for all modules.
- Coupling numeric APIs validate their documented domains. Zero denominators, negative
  physical parameters, invalid dimensions, NaN, and infinity return NaN; valid zero
  limits such as zero density, temperature, radius, or rate return zero.
- Functions returning numeric results generally do not return an error code. Invalid
	inputs may produce NaN, infinity, a mathematically meaningless value, or an
  implementation failure.
- Coupling APIs return NaN for invalid numeric inputs, so callers should check
  `std::isfinite` (or equivalent) on returned values.
- Future API work should introduce consistent validation and an error-reporting
  policy rather than relying on these implementation details.

## Units and numerical conventions

- PhysicsEngine coupling functions use SI inputs and return SI quantities unless the
  header explicitly states another unit.
- `pe_fission_thermal_power` accepts fission energy in MeV and returns watts when
  the fission rate is in events per second.
- Quantum and module-specific APIs must be read with their module headers because
  some functions use joules, electron-volts, metres, kelvin, or dimensionless
  quantities.
- Floating-point results should be compared with tolerances, not exact equality.
- Coupling units are documented in `PhysicsCoupling.h`: dimensionless Nu/reflectance,
  W/m^2 heat flux, metres for skin depth, joules for quantum/nuclear energies, m/s
  terminal velocity, and watts for fission power.

## Thread-safety

- The module C APIs are not currently documented as thread-safe. Treat individual
  handles as thread-confined unless a module explicitly guarantees otherwise.
- Independent stateless calls may be suitable for concurrent use, but this has not
  yet been formally tested for every module.
- The async system is process-global and owns shared worker, queue, completion, and
  pending-task state. Initialization, submission, and shutdown lifecycle transitions
  are serialized internally.
- `pe_async_init` should be called before submission. Repeated initialization is a
  no-op; `numThreads <= 0` selects hardware concurrency with a fallback.
- `pe_async_shutdown` waits for queued/running work, joins workers, clears completion
  records, and is idempotent. Calling shutdown from inside a task callback is not
  supported.
- The callback receives caller-owned `void*` data; the async system does not copy or
  free that data.
- Submitting before initialization or with a null callback returns task ID 0 and
  performs no work. ID 0 is immediately complete, so waiting on it cannot block.
- Callback exceptions are caught by the worker. The existing C API has no exception
  transport, so the task is marked complete without exposing the exception.
- Completed task records are retained up to 4096 entries and older completed records
  are evicted when possible. Unknown or evicted IDs are treated as complete.
- `pe_tasks_pending` is a concurrent snapshot and may change immediately after it
  returns.
- The returned pointer from name APIs such as `cp_getName` may use a thread-local
  buffer and should be copied by the caller if it must survive subsequent calls.

## Compatibility policy

- Preserve existing exported names and signatures until a versioned API change is
  intentionally approved.
- Correct documentation first when the README disagrees with actual symbols.
- Add aliases only when they do not create ambiguous ownership or maintenance cost.
- Any future error/status redesign should be additive or versioned rather than
  silently changing numeric-return behavior.

## Packaging and external consumption

- The supported Phase 7 artifact is the static `PhysicsEngine` library.
- Installation provides `include/PhysicsEngine.h`, the module C API and export headers,
  `include/physics/` collision headers, `lib/PhysicsEngine.lib` on MSVC, and a CMake
  package under `lib/cmake/PhysicsEngine/`.
- External CMake projects should use `find_package(PhysicsEngine CONFIG REQUIRED)` and
  link `PhysicsEngine::PhysicsEngine`.
- Sibling repositories are build-time inputs only; installed consumers do not need them.
- Shared-library packaging is deferred pending an explicit ABI and runtime dependency policy.
