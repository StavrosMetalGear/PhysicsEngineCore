# Release Checklist

## Source and versioning

- [x] Update `project(... VERSION ...)` in `CMakeLists.txt`.
- [x] Add the release entry to `CHANGELOG.md`.
- [ ] Confirm the tag is `vMAJOR.MINOR.PATCH` and matches the CMake project version.
- [x] Review `API_CONTRACT.md`, `README.md`, and public headers for contract changes.

## Dependencies

- [x] Verify all eight revisions in `dependencies/PhysicsCoreRepos.cmake`.
- [x] Confirm each pinned revision is represented by a full immutable commit revision.
- [x] Build from clean local trees with no sibling sources inside the repository.

## Build and test

- [x] Configure and build Debug with MSVC/Ninja.
- [x] Configure and build Release with MSVC/Ninja.
- [x] Run CTest with no failures in both configurations.
- [x] Enable `PHYSICSCORE_ENABLE_STRICT_WARNINGS` and resolve project-owned warnings.
- [ ] Run the formatting workflow remotely.

## Package and consumer

- [x] Install the Release package to a clean prefix.
- [x] Inspect the static library, headers, and CMake package metadata.
- [x] Configure, build, and run `examples/consumer` against the install prefix.
- [x] Verify the consumer does not require sibling repository paths.
- [ ] Upload the versioned package artifact from the release workflow.

## Documentation and release

- [x] Generate API documentation with `docs/Doxyfile`.
- [x] Review generated public API documentation configuration.
- [x] Confirm `LICENSE.txt` is included in the source release.
- [ ] Create the annotated Git tag and publish release notes.

## Phase 11 compatibility review for each release candidate

- [ ] Record the candidate commit, compiler/toolset, architecture, build configuration,
	  runtime settings, and all eight sibling revisions with the release evidence.
- [ ] Review the installed header manifest and changes against the last released
	  package; explicitly classify additions, removals, and compatibility impact.
- [ ] Run the C/C++ signature, capability, layout, installed-header, package-version,
	  installed consumer, and relocation checks appropriate to the release target.
- [ ] Regenerate and review the module declaration inventory; account for limitations
	  of the regex parser and independently review public declaration changes.
- [ ] Inspect each release archive with a tool appropriate to its platform and retain
	  the symbol list together with the corresponding package and build provenance.
- [ ] For an MSVC x64 static package, run `scripts/inspect-msvc-symbols.ps1` on the
	  actual candidate archive and retain its output and symbol list. Windows CI
	  currently covers its own installed artifact, not every possible release artifact.
- [ ] Obtain the actual previous-release artifact or verified symbol list, record its
	  provenance, and compare comparable builds. Review removals and additions; name
	  comparisons alone do not validate signatures, layouts, behavior, or ABI.
- [ ] Review version changes, deprecations, handle/callback/result behavior, and
	  compatibility notes against the approved policy once that policy exists.
- [ ] Investigate and document any failing check before publishing; do not mark the
	  Phase 11 checkpoint complete solely because the current CTest suite passes.

## Phase 9 local audit

- Debug: `out/phase9-debug`, all 15 tests passed.
- Release: `out/phase9-release`, strict warnings enabled, all 15 tests passed.
- Install prefix: `out/phase9-install`, 35 expected files, no unexpected runtime/build artifacts.
- External consumer: configured, built, and ran successfully with output `1.0.0 consumer OK`.
- Local CI confirmation and tag publication remain pending external/manual actions.
