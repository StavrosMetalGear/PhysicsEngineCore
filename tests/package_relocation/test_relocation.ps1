[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$PackagePrefix,
	[Parameter(Mandatory = $true)]
	[string]$TestRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$package = (Resolve-Path -LiteralPath $PackagePrefix).Path
if (-not (Test-Path -LiteralPath (Join-Path $package 'lib\cmake\PhysicsEngine\PhysicsEngineConfig.cmake') -PathType Leaf)) {
	throw "PhysicsEngine package config not found under $package"
}
if (Test-Path -LiteralPath $TestRoot) { throw "Refusing to overwrite an existing test directory: $TestRoot" }
$null = New-Item -ItemType Directory -Path $TestRoot
$root = (Resolve-Path -LiteralPath $TestRoot).Path
$relocated = Join-Path $root 'relocated package'
New-Item -ItemType Directory -Path $relocated | Out-Null
Copy-Item -LiteralPath (Join-Path $package 'include') -Destination $relocated -Recurse
Copy-Item -LiteralPath (Join-Path $package 'lib') -Destination $relocated -Recurse
$build = Join-Path $root 'consumer build'

& cmake -S $PSScriptRoot -B $build -G Ninja '-DCMAKE_BUILD_TYPE=Release' "-DPACKAGE_PREFIX=$relocated"
if ($LASTEXITCODE -ne 0) { throw 'Relocated package configuration failed.' }
& cmake --build $build --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Relocated consumer build failed.' }
& ctest --test-dir $build --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Relocated consumer tests failed.' }
Write-Output "Relocated package checks passed. Fixture retained at $root"
