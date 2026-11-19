# Phase 10.5 Performance Baseline

## Scope and priorities

Correct, scientifically meaningful results and the existing public contract come
first. Performance is a top priority; allocation-free steady state and fewer cache
misses are goals where feasible, not guarantees for every operation. The outstanding
scientific discrepancies in `PHASE10_API_AUDIT.md` remain correctness work. Phase 10's
811/811 declaration-reference coverage does not establish scientific correctness.

## Initial exploratory PC trace

This observation starts the investigation; it is **not** a representative Release
benchmark, an allocation baseline, or evidence for an optimization.

| Field | Observation |
|---|---|
| Date | 2026-11-19, local time |
| Repository revision | `34023e3` |
| CPU | AMD Ryzen 7 4800H with Radeon Graphics |
| Process | `PhysicsEngineTest`, the startup smoke executable |
| Profiler | Visual Studio CPU sampling, reported rate 1,000 samples/second |
| Trace duration | 4,692.0745 ms |
| CPU samples | 24 across the process; the reported hottest path contains 10 |
| Build configuration | Not verified for the profiled process; the existing CMake cache is MSVC x64 Debug |

The smoke executable exercises metadata, selected Quantum, Fluid, Classical and
Nuclear calls, a single async task, and coupling. It is not a controlled workload
for all eight modules, collision, or async throughput. The hottest reported path
was unresolved `ntdll` code, and the short run supplied too few samples to identify
a meaningful engine hotspot. CPU sampling does not measure native allocation counts
or cache misses. No production code was changed based on this observation.

An attempt to generate a broadphase performance test produced a separate Visual
Studio project outside the repository; its Debug build failed because `physics/AABB.h`
was not on the include path. It was not run and provides no baseline numbers. A
buildable in-repository measurement artifact is still needed.

## Remaining measurements

- Create repeatable, correctness-checked Release workloads for the eight modules,
  broadphase, narrowphase, and async. Record inputs, expected results, warmup,
  iterations, timing distributions, compiler settings, revisions, and machine data.
- Measure native allocation counts and bytes separately for setup, steady state,
  and teardown; record instrumentation limitations and any allocation behavior it
  cannot observe. Measure cache behavior only with suitable platform tooling.
- Collect PC data for each workload. Android build, hardware, and measurements are
  not yet validated and remain pending; do not infer Android results from PC data.
- Use the same workload and configuration before and after each optimization, run
  correctness tests, and defer hard regression gates until noise is understood.
