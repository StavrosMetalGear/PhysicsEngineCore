# PhysicsEngineCore Public API Contract

## Scope

This document records the public contract observed in the current source tree. The
module headers in the sibling repositories remain the authoritative declaration
sources; this file explains how they are aggregated and what callers may currently
rely on.

The exported module APIs use C linkage and are intended to be callable from C and
C-compatible consumers. The collision subsystem under `include/physics` is C++17
and is not part of the C ABI.

## Phase 11 compatibility work

The installed header list in `CMakeLists.txt` and the installed `include/physics`
directory describe the current package surface, not a finalized stability promise.
`PhysicsEngine.h` aggregates the eight module declarations, async, coupling, and
engine metadata. `PhysicsMetadata.h` can also be included independently for the
engine metadata functions, C API revision macros, and capability bits. The module
declaration headers originate from the sibling
repositories except for the local Fluid header. `PhysicsEngine_export.h` and the
module export headers support these declarations; internal implementation headers
are not installed by this project. The installed collision headers form a separate
C++17 surface. Classifying each surface and approving compatibility guarantees
remain Phase 11 work.

`PHASE11_COMPATIBILITY_PROPOSAL.md` groups the proposed surface boundaries,
source/binary/package rules, and release decisions for maintainer review. It is a
draft, not an approved policy or a replacement for the observed contracts below.

### API revision and capabilities

`PE_API_VERSION_MAJOR`, `PE_API_VERSION_MINOR`, and `PE_API_VERSION_PATCH` identify
the C API revision declared by the headers, currently 1.0.0. This initial revision
is provisional; the versioning and compatibility policy is not yet finalized.
It is a compile-time header value, not proof that a separately supplied binary
matches those headers. `pe_version()` reports the engine release version instead.

The installed CMake package currently reports project version `1.0.0` and uses
CMake's `SameMajorVersion` selection rule: a non-exact request must have the
same major version and must not be newer than the package. `EXACT` requires the
same version string, so `1.0` does not match `1.0.0`. CMake also checks pointer
size when both package and consumer provide it. This selection rule is not proof
of source or ABI compatibility and is not the finalized versioning policy.
Windows CI checks representative accepted and rejected requests against the
installed package. In an MSVC x64 developer environment, use a fresh test path:

```powershell
cmake -DTEST_ROOT="<new-test-directory>" -DPACKAGE_PREFIX="<install-prefix>" -P tests/package_version/test_version.cmake
```

`pe_capabilities()` returns a `uint64_t` bitmask describing the metadata, async,
coupling, and eight module C APIs compiled into this library. Check the named
`PE_CAPABILITY_*` bits individually and ignore unknown bits. Existing bit values
must not be reassigned; additional capabilities can use unused bits. A set bit
does not establish async initialization, support for every equation in a sibling
repository, numerical correctness, allocation behavior, or platform performance.
The separate C++ collision headers are not represented in this C API bitmask.
This function is a new symbol: earlier builds reporting engine version 1.0.0 do
not necessarily provide it, so callers must link against a matching newer library.

`tests/baselines/capability_snapshot.h` records the 11 currently assigned bit
values independently of the public macros. C11 and C++17 compile checks compare
both build-tree and installed headers with this snapshot; the metadata runtime
test checks that the current library reports all recorded bits while allowing
additional bits. Windows CI runs these checks and negative compile probes:

```powershell
cmake -S tests/capability_baseline -B out/phase11-capability-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release
```

The two negative probes intentionally fail after changing a snapshot value.
This detects reassignment or removal of listed values, not newly added duplicate
bits, semantic changes, or compatibility with a previous release.

### Deprecation annotation

`PHYSENG_DEPRECATED(message)` is available in `PhysicsEngine_export.h` for future
function declarations; place it before `PHYSENG_API`. It requests a compiler
warning with the supplied message on MSVC, GCC, and Clang, and is a no-op on other
compilers. It does not remove a symbol or change its calling convention. No API
has been deprecated by adding this macro, and warning periods and removal rules
remain to be decided.

The standalone `tests/deprecation` CMake project verifies both unused declarations
and diagnostics for uses in C11 and C++17, with static, shared-export, and
shared-import macro configurations. Run it in a compiler environment with:

```powershell
cmake -S tests/deprecation -B out/phase11-deprecation -G Ninja -DCMAKE_BUILD_TYPE=Release
```

The tests treat the expected deprecation warnings as errors; the six reported
failed compile probes are intentional. Shared-mode annotation checks do not build
or validate a shared-library package. Windows CI runs these checks with MSVC.

`tests/test_api_compat_cpp.cpp` is an initial compile-only C++17 check of selected
declarations from all eight modules and representative metadata, task, callback,
and coupling types. It detects changes to these source signatures when built; it
does not check C compilation, linking, exported symbols, object layout, behavior,
or ABI compatibility.

`tests/baselines/engine_api_snapshot.h` records the 20 engine-owned metadata,
async, and coupling signatures observed at revision `d813a60`. Compile-only C11
and C++17 targets compare current declarations with that snapshot. The standalone
`tests/signature_baseline` CMake project also checks that a deliberately altered
signature is rejected; its two failed compile probes are intentional. Windows CI
runs both the build targets and these probes. This snapshot is not a previous
release or an approved stable-surface definition. It does not cover the module
APIs, new declarations, exports, calling conventions on every toolchain, layouts,
behavior, or cross-release ABI compatibility.

`tests/baselines/quantum_records_msvc_x64.h` is a provisional layout snapshot
of the four Quantum C records from pinned QuantumCore revision
`529d5912af6f2e15f1f1a2b04b82f029b6c4e90c`. MSVC x64 C11 and C++17
compile-only targets check their sizes, alignments, and listed field offsets and
types under the default packing configuration. The C check is enabled by
`PHYSICSCORE_ENABLE_C_API_CHECKS`; the C++ check is always built on MSVC x64.
Windows CI also runs positive and deliberately changed-offset compile probes:

```powershell
cmake -S tests/quantum_layout_baseline -B out/phase11-quantum-layout -G Ninja -DCMAKE_BUILD_TYPE=Release
```

Set `PHYSICSCORE_REPOS_DIR` if the sibling repositories are not in the default
location. The two failed negative probes are intentional. The installed-baseline
project also tests a copied installed Quantum header whose `QP_EnergyLevel2D::nx`
type is changed from `int` to `unsigned int`: C and C++ control probes verify the
fixture's type, size, alignment, and offsets, and the snapshot checks must reject it.
These probes run only for MSVC x64. This snapshot is not a
previous release or a stability guarantee. It does not cover other records,
platforms, nondefault packing, newly added fields that fit existing padding,
function calling conventions, or runtime and cross-release ABI compatibility.

`tests/test_api_compat_c.c` is a representative C11 consumer check. It includes
`PhysicsEngine.h`, checks selected function and task types, links selected symbols
from all eight modules and engine layers against the static library, and executes
metadata and Fluid smoke checks. Configure with
`-DPHYSICSCORE_ENABLE_C_API_CHECKS=ON`, then build and run CTest; this option is
off by default so existing C++-only builds do not acquire a C compiler requirement.
The Windows CI configuration enables it. The test uses the build-tree headers and
does not validate the installed package, every API, cross-release ABI compatibility,
structure layout, or shared-library exports.

`examples/c_consumer` separately checks a C11 source file against installed headers
and the installed static package. It exercises metadata, Fluid, and Quantum handle
lifecycle calls; both installed-package examples also compile `PhysicsMetadata.h`
in a separate translation unit. `examples/consumer` provides the C++ package check. The C
example uses a C++ linker because the library contains C++ implementation code.
Windows CI builds both examples; this is representative package coverage, not a
guarantee for every function, ABI, platform, or shared-library configuration.

`tests/package_relocation/test_relocation.ps1` copies an installed package's
`include` and `lib` directories to a new path containing spaces, verifies the
imported Release target points to the copied headers and archive, then builds
and runs both examples against the copy. Windows CI runs this MSVC x64 check.
From an MSVC developer environment, use a fresh test directory:

```powershell
./tests/package_relocation/test_relocation.ps1 -PackagePrefix "<install-prefix>" -TestRoot "<new-test-directory>"
```

This checks representative static-package relocation, not other configurations,
platforms, toolchains, shared libraries, or cross-release ABI compatibility.

`tests/installed_baseline` checks the same engine signature snapshot against the
installed package in C11 and C++17 and checks the Quantum layout snapshot on MSVC
x64. It also compiles representative C++ API types, links and runs the
representative C API check, and runs the C async callback lifecycle test against
the installed static library. A deliberately altered copy of the installed
metadata header verifies that both languages reject a changed signature. After
installing the package, run:

```powershell
cmake -S tests/installed_baseline -B out/installed-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<install-prefix>"
cmake --build out/installed-baseline
ctest --test-dir out/installed-baseline --output-on-failure
```

The two negative compile probes are expected to fail. Windows CI runs these checks
against its installed static package. They do not compare a previous release,
check every installed declaration, or prove that every header matches the linked binary.

`tests/installed_headers` independently compiles each of the 20 currently listed
installed C API and export headers as C11 and C++17, and each of the 11 currently
listed collision headers as C++17. It uses only the package target's include paths.
Windows CI builds this check against the installed package. To run it locally:

```powershell
cmake -S tests/installed_headers -B out/installed-headers -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<install-prefix>"
cmake --build out/installed-headers
```

The configure step compares every file in the installed include directory with
`tests/installed_headers/header_manifest.cmake` and fails on missing or unexpected
files. Use a clean, dedicated install prefix; stale or unrelated files also fail
the check. Review and update the manifest when the installed surface intentionally
changes. CI exercises matching, missing, unexpected, and duplicate-entry fixtures.
The manifest is a current package inventory, not a stability promise or a previous
release comparison. Independent inclusion does not validate function prototypes, linking, behavior, or ABI
compatibility; in particular, the Solid State C non-prototypes noted below remain.

On Windows, `scripts/inspect-msvc-symbols.ps1` uses `dumpbin /linkermember:1` to
inspect the installed MSVC x64 static library. It checks the module declarations
listed in `PHASE10_API_INVENTORY.md` and engine declarations in `PhysicsMetadata.h`,
`PhysicsAsync.h`, and `PhysicsCoupling.h`. Specify `EngineHeadersPath` to inspect
the candidate package headers; it defaults to the current source tree. In an MSVC
developer environment, run:

```powershell
./scripts/inspect-msvc-symbols.ps1 -LibraryPath out/phase11-install/lib/PhysicsEngine.lib -EngineHeadersPath out/phase11-install/include -SymbolListPath out/phase11-symbols.txt
./tests/test_inspect_msvc_symbols.ps1 -LibraryPath out/phase11-install/lib/PhysicsEngine.lib -EngineHeadersPath out/phase11-install/include
```

Windows CI checks the installed library against its installed engine headers and
the module inventory, then uploads the sorted archive symbol list. Engine names
are extracted with a regex, which can miss declarations or include inactive code.
The inspector validates eight distinct, nonempty module declaration lists and
rejects duplicate names within a list; this does not prove inventory completeness.
That list includes implementation and toolchain symbols; it is not a public API
manifest or a DLL export list. The inspector writes the requested list only after
all expected names are present; if verification fails, an existing list is left
untouched and may be stale. Use a fresh output path for each artifact. The
generated module inventory is a snapshot and
must be regenerated and reviewed when declarations change. Windows CI compares
declaration names from both source and installed module headers with the checked-in
inventory before inspecting the archive. The audit still reads sibling source
definitions for its other reports. To run the matching and deliberately altered
inventory and installed-header fixtures locally, use a fresh path:

```powershell
./tests/test_c_api_inventory.ps1 -ReposRoot "<sibling-repositories-directory>" -HeadersRoot "<install-prefix>/include" -TestRoot "<new-test-directory>"
```

The audit uses a regex-based parser, not a C/C++ compiler: it can miss or
misinterpret declarations and does not check signatures or preprocessing.
Consequently, a matching inventory is not proof of completeness. Name presence does not
verify signatures, behavior, structure layout, cross-release ABI compatibility, or
shared-library exports; a previous-release comparison and stable-surface policy
remain open.

`scripts/compare-msvc-c-symbols.ps1` compares two such archive symbol lists. It
reports additions and fails on removals of undecorated names with the engine or
module prefixes. It rejects recognizable leading-underscore and `@N` decorated
candidate C names rather than silently ignoring them; other names are ignored.
This check does not identify every decoration or determine the architecture. For
example, after obtaining the list
from an actual previous release and generating the current list, run:

```powershell
./scripts/compare-msvc-c-symbols.ps1 -BaselinePath previous-release-symbols.txt -CurrentPath out/phase11-symbols.txt
```

Use lists from comparable MSVC x64 builds and verify their provenance and
completeness. The family checks reject some incomplete inputs but cannot prove a
list is complete. Prefixes can include implementation symbols, so differences
need review against the eventual public API classification. The script compares
names only; unchanged names do not prove compatible signatures, structure layouts,
calling conventions, behavior, or binary compatibility. CI tests the comparison
logic with fixtures, but does not yet compare against a real previous release.

`scripts/compare-package-headers.ps1` compares relative file names and SHA-256
content hashes in two installed `include` trees. It reports added and changed
files and fails on removed files, empty trees, or links. Once a verified previous
release package is available, compare it with a clean candidate install:

```powershell
./scripts/compare-package-headers.ps1 -BaselineInclude "<previous-prefix>/include" -CurrentInclude "<candidate-prefix>/include"
```

CI exercises matching, added, changed, removed, and empty-tree fixtures; it does
not compare a real previous release. Hash differences include comments and other
non-API edits; unchanged files do not establish ABI or behavioral compatibility.
Inspect changes manually and verify both trees' provenance and completeness.

`RELEASE_CHECKLIST.md` lists the Phase 11 release-candidate review steps, including
recording build provenance, inspecting the actual release artifacts, and comparing
against a verified previous release. It is a manual checklist, not an automated
release gate or an approved cross-release compatibility policy.

Solid State declares `ss_flux_quantum()` and `ss_conductance_quantum()` with empty
parameter lists, which are non-prototypes in C before C23. These functions are not
covered by the C signature checks. A broader C consumer audit remains necessary
before promising C compatibility.

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

`PHASE10_API_CATEGORIES.md` provides the generated Phase 10.1 planning view of
all 811 declarations. Its categories describe testing strategy only; they do
not expand the behavioral guarantees of an API.

`PHASE10_API_TEST_PLAN.md` records exact references from the current C++ tests.
An API marked tested there has a call-site reference, but callers must still
consult the module-specific domain and ownership rules before relying on it.
The current plan records 811 referenced declarations out of 811; this is an
exact-symbol milestone, not an assertion that every exported equation is complete.

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

The current test suite validates reference values and safe boundary behavior for
all five functions. The remaining module APIs have representative integration
coverage, including Solid State lattice, diffraction, thermal-transport, and
Landauer reference checks, plus representative Thermodynamics and Optics
equation and domain checks. Exhaustive equation coverage is still being added
incrementally. Representative Electrodynamics and Nuclear reference/domain
checks are also included; Nuclear SEMF returns zero for nonpositive charge or
mass-number inputs according to the current implementation.
Quantum and Classical scalar reference checks are also included. The Quantum
harmonic-oscillator energy helper currently follows the pinned implementation's
dimensionless `hbar = 1` convention; callers should not assume SI energy units
for that function without a future API clarification.

The generated Phase 10 test plan currently records 811 referenced declarations out
of 811; this is measured exact-symbol coverage, not proof that every API domain,
buffer contract, numerical solver, or export path is fully validated.

Classical N-body energy and angular-momentum wrappers take seven doubles per
body in the order `x, y, z, vx, vy, vz, mass` and copy them into an internal
vector. For a positive body count, supply a valid array with at least seven
values per body. Their reference checks do not establish allocation-free or
real-time behavior.

Array-output APIs write into caller-owned buffers. Callers must allocate enough
space for the requested output. `qp_momentum_space_wavefunction_1d_box` requires
space for `2 * numPoints` doubles for positive `numPoints`, interleaved as real
and imaginary components; it returns the complex sample count. A null output
is accepted for a count query, but the current implementation still computes
and allocates the internal vector. The momentum grid is fixed by the
implementation and the integral uses 200 position samples.
`qp_transfer_matrix_multilayer` requires valid width and height arrays with
at least `numLayers` entries for positive counts. At positive energy, avoid
layers with height exactly equal to the energy: the implementation divides by
zero at that threshold. `qp_export_transfer_matrix_csv` has the same array
requirements and threshold issue; its wrapper copies the arrays into internal
vectors. The pinned Quantum two-level exact and
perturbative wrappers write the positive energy branch to the first output and
the negative branch to the second, despite the sibling header's reversed labels.
The Quantum `qp_free_electron_transition` wrapper writes the energy gap to its
first non-null output and wavelength `h*c/gap` to its second, although the sibling
header labels the second output as transition energy. Use positive electron counts
and nonzero mass and length; the output labeling requires an API correction.
For `qp_export_spin_analysis_csv`, use a normalized nonzero spinor: the current
implementation normalizes probabilities but not the spin expectation values,
and a zero spinor leads to division by zero.
Nuclear paired-vector wrappers return the
number of entries written, capped by the supplied buffer length; they do not
allocate caller buffers. Use nonnegative capacities and provide valid pointers
for both outputs when capacity is positive. Quantum `qp_list_allowed_j` and `qp_list_allowed_f`
return the total available count even when the supplied buffer is shorter;
they write at most the specified capacity and support a null buffer for count queries.
The Quantum `qp_list_energy_levels_2d_box`, `qp_list_energy_levels_3d_box`,
`qp_compute_hyperfine_levels`, `qp_compute_zeeman_levels`, and
`qp_compute_paschen_back_levels` wrappers have the same total-count and
bounded-write behavior. For `qp_bloch_evolution`, use positive `numSteps` and
allocate at least `numSteps + 1` doubles for each non-null output array; the
implementation uses explicit Euler steps. `qp_slater_determinant_isw` requires
at least as many valid position entries as orbital entries.

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

### Current handle, task, callback, and result baseline

This describes the current interface and informs future compatibility decisions;
it is not a finalized cross-release ABI guarantee.

- The six handle typedefs are currently `void*`. They are opaque to callers, must
  be used with their matching module and destroy function, and must not be
  dereferenced, serialized, or treated as interchangeable just because their C
  types match. Pointer representation depends on the target ABI.
- `PE_TaskId` is currently `uint64_t`. Treat nonzero IDs as opaque process-local
  tokens rather than persistent identifiers or sequence numbers. ID zero denotes
  a rejected submission and is immediately complete.
- Unknown, evicted, and cleared IDs are currently treated as complete. Thus
  `pe_task_ready` or `pe_task_wait` cannot establish that an unknown ID was ever
  submitted or that its callback ran successfully. `pe_tasks_pending` is a snapshot,
  not an individual task result.
- `PE_TaskFunc` currently has the signature `void (*)(void*)`; its data remains
  caller-owned and must outlive the callback. Coordinate shutdown with submissions
  and callback lifetimes. A completed callback does not have a result or error
  channel; C++ exceptions are caught internally and are not reported to callers.
- Module numeric results and error handling are not uniform. In particular, do not
  interpret every zero or NaN as a common status code, or assume scientific validity
  from a successful call; consult the module contract and Phase 10 audit.

The opt-in `test_async_c_contract` checks a C callback, zero and null submission
behavior, waiting, caller-owned data, and shutdown draining in a controlled run.
The opt-in `test_async_mixed_contract` additionally submits a callback compiled
as C from a C++ caller and checks its completion and user-data pointer. Windows
CI runs the mixed-language test against both build-tree and installed libraries.
The C and C++ compatibility checks also cover the current async signatures and
handle typedefs. They do not prove all thread interleavings, persistence, platform
ABIs, or future compatibility.

## Packaging and external consumption

- The supported Phase 7 artifact is the static `PhysicsEngine` library.
- Installation provides `include/PhysicsEngine.h`, `include/PhysicsMetadata.h`, the module C API and export headers,
  `include/physics/` collision headers, `lib/PhysicsEngine.lib` on MSVC, and a CMake
  package under `lib/cmake/PhysicsEngine/`.
- External CMake projects should use `find_package(PhysicsEngine CONFIG REQUIRED)` and
  link `PhysicsEngine::PhysicsEngine`.
- Sibling repositories are build-time inputs only; installed consumers do not need them.
- Shared-library packaging is deferred pending an explicit ABI and runtime dependency policy.
