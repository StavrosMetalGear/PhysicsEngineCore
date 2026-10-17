# Changelog

All notable changes to PhysicsEngineCore are documented here.

## Versioning policy

PhysicsEngineCore follows semantic versioning: `MAJOR.MINOR.PATCH`.

- MAJOR: incompatible public API, ABI, package layout, or contract changes.
- MINOR: backward-compatible public features or package additions.
- PATCH: backward-compatible fixes, documentation, tests, and build corrections.

Release tags must match the project version in `CMakeLists.txt` and the installed
`PhysicsEngineConfigVersion.cmake` package metadata.

## Unreleased

- Added Phase 8 CI and release-quality automation.
- Added pinned sibling-repository revisions for reproducible aggregation builds.
- Added Windows MSVC/Ninja build, test, install, and external-consumer workflow.
- Added formatting validation and opt-in strict warnings.

## 1.0.0

- Unified eight physics sibling repositories into the PhysicsEngine static library.
- Added async execution, coupling APIs, collision headers, CMake installation, and
  relocatable external-consumer package support.
