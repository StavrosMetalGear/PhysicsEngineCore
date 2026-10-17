# Release Checklist

## Source and versioning

- [ ] Update `project(... VERSION ...)` in `CMakeLists.txt`.
- [ ] Add the release entry to `CHANGELOG.md`.
- [ ] Confirm the tag is `vMAJOR.MINOR.PATCH` and matches the CMake project version.
- [ ] Review `API_CONTRACT.md`, `README.md`, and public headers for contract changes.

## Dependencies

- [ ] Verify all eight revisions in `dependencies/PhysicsCoreRepos.cmake`.
- [ ] Confirm each pinned revision is reachable from its configured repository URL.
- [ ] Build from a clean checkout with no untracked sibling sources.

## Build and test

- [ ] Configure and build Debug with MSVC/Ninja.
- [ ] Configure and build Release with MSVC/Ninja.
- [ ] Run CTest with no failures in both configurations.
- [ ] Enable `PHYSICSCORE_ENABLE_STRICT_WARNINGS` and resolve project-owned warnings.
- [ ] Run the formatting workflow.

## Package and consumer

- [ ] Install the Release package to a clean prefix.
- [ ] Inspect the static library, headers, and CMake package metadata.
- [ ] Configure, build, and run `examples/consumer` against the install prefix.
- [ ] Verify the consumer does not require sibling repository paths.
- [ ] Upload the versioned package artifact.

## Documentation and release

- [ ] Generate API documentation with `docs/Doxyfile`.
- [ ] Review generated public API documentation.
- [ ] Confirm `LICENSE.txt` is included in the source release.
- [ ] Create the annotated Git tag and publish release notes.
