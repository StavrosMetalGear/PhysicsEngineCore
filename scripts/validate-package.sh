#!/usr/bin/env bash
set -euo pipefail

build_dir=${1:-out/release-quality}
install_dir=${2:-out/release-quality-install}
repos_dir=${3:-..}

cmake -S . -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release -DPHYSICSCORE_REPOS_DIR="$repos_dir"
cmake --build "$build_dir" --parallel
ctest --test-dir "$build_dir" --output-on-failure
cmake --install "$build_dir" --prefix "$install_dir"
cmake -S examples/consumer -B "${build_dir}-consumer" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(realpath "$install_dir")"
cmake --build "${build_dir}-consumer" --parallel
"${build_dir}-consumer/physics_engine_consumer"
