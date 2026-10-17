param(
	[string]$BuildDirectory = "out/release-quality",
	[string]$InstallDirectory = "out/release-quality-install",
	[string]$ReposDirectory = ".."
)

$ErrorActionPreference = "Stop"

cmake -S . -B $BuildDirectory -G Ninja -DCMAKE_BUILD_TYPE=Release -DPHYSICSCORE_REPOS_DIR=$ReposDirectory
cmake --build $BuildDirectory --parallel 2
ctest --test-dir $BuildDirectory --output-on-failure
cmake --install $BuildDirectory --prefix $InstallDirectory
cmake -S examples/consumer -B "$BuildDirectory-consumer" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=(Resolve-Path $InstallDirectory)
cmake --build "$BuildDirectory-consumer" --parallel 2
& ".\$BuildDirectory-consumer\physics_engine_consumer.exe"
if ($LASTEXITCODE -ne 0) {
	throw "External consumer failed with exit code $LASTEXITCODE"
}
