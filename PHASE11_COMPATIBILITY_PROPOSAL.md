# Phase 11 compatibility policy proposal

**Status: draft; maintainer approval required.** This proposal does not change the
current API or make an existing build a released compatibility baseline. It is
intended to resolve roadmap items 98–106 together rather than treating test
coverage as a stability promise. `API_CONTRACT.md` remains the reference for
observed behavior and known limitations.

## Proposed scope and boundaries

| Surface | Proposed treatment | Decision or work still required |
|---|---|---|
| Engine-owned C metadata, async and coupling declarations in `PhysicsMetadata.h`, `PhysicsAsync.h` and `PhysicsCoupling.h` | Candidate stable C source interface; `PhysicsEngine_export.h` supplies declaration annotations | Review every declaration, ownership, calling convention and documented behavior before approval |
| Module C declarations and records in installed module headers, including local `fluid_c_api.h` | Candidate public source interface; declarations originate in the pinned sibling repositories except Fluid | Agree on an exact stable symbol/type list with module maintainers; address known C prototype and scientific-contract gaps |
| `PhysicsEngine.h` | Candidate public umbrella over the approved C interfaces | Decide whether its transitive dependencies and inclusion behavior are guaranteed |
| `include/physics/` | Separate C++17 source interface, not part of the C ABI | Decide whether any C++ surface or layout receives a separate guarantee |
| Implementation sources, non-installed headers and incidental static-archive symbols | Implementation detail; do not infer a public guarantee from symbol visibility | Review exceptions explicitly before approval |

The installed header manifest and static archive inventory describe current
contents, not an approved list of stable declarations. Sibling repositories can
change independently; a PhysicsEngineCore release must record and review the
exact revisions it incorporates.

## Proposed compatibility rules

- **Source:** For approved declarations, a compatible release should allow a
  conforming consumer using the previously documented API to recompile without
  source changes. Additions must not silently alter existing meaning. Source
  compatibility must be assessed separately for C and C++.
- **Binary:** A guarantee requires a named platform/architecture, calling
  convention, compiler/toolset and runtime configuration, and an identified pair
  of releases. For a static library, assess whether previously compiled consumer
  objects can be relinked with the new archive; an already linked executable is
  not updated by replacing the archive. Do not promise binary compatibility
  across platforms, toolchains, runtime settings or shared libraries by default.
- **Package:** Define supported install paths, imported targets, library type and
  dependencies per release. Verify consumers against a clean installed package;
  CMake version selection and relocation checks are evidence, not proof of API
  or ABI compatibility.
- **Versioning:** `CHANGELOG.md` describes semantic-versioning intent, but its
  application to the approved surface and binary matrix still needs a decision.
  The current header API revision, engine version and CMake package version have
  distinct meanings. In particular, earlier engine version `1.0.0` binaries may
  lack `pe_capabilities`; matching version strings alone are insufficient.
- **Deprecation:** A deprecation should name a replacement and planned removal,
  retain the old behavior and symbol during its warning period, and appear in
  release notes. The duration, exceptions and removal rules are not approved.
- **Handles and callbacks:** Keep ownership, callback signatures, user-data
  lifetime, task-ID semantics and error/result conventions explicit. Any change
  in those contracts requires a compatibility review; types alone cannot prove
  equivalent behavior. Unknown task IDs currently report complete and must not
  be used as evidence that work ran.

## Release evidence and decisions

1. Approve the stable declaration/type inventory, supported platforms and
   toolchains, and whether C++ collision interfaces are in scope.
2. Approve versioning, deprecation duration and removal rules, and the handle,
   callback, task and result compatibility guarantees.
3. Obtain an actual previous release artifact with trustworthy provenance, or
   explicitly decide how to establish a first baseline if none exists. The local
   Git checkout currently has no release tags; that alone does not establish that
   no release exists elsewhere.
4. Compare the candidate and baseline using matching build configurations, review
   symbols, declarations, relevant layouts, behavior and installed package, and
   document any accepted break and version consequences. The name comparison tool
   and provisional snapshots are only partial evidence.
5. Enforce inspection in the actual release process and record the results using
   `RELEASE_CHECKLIST.md`. Windows CI currently checks its own package, not every
   release artifact.

Open decisions or missing evidence block the Phase 11 checkpoint; the current
23-test Release x64 suite does not establish cross-release compatibility.
