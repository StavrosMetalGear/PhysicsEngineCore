Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script = Join-Path $root 'scripts\write-compatibility-evidence.ps1'
$temp = Join-Path $root ('out\compatibility-evidence-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null

try {
	$repos = Join-Path $temp 'repos'
	New-Item -ItemType Directory -Path $repos | Out-Null
	$names = @('PhysicsEngineCore', 'QuantumCore', 'ClassicalMechanicsCore', 'ElectrodynamicsCore',
		'FluidDynamicsCore', 'NuclearCore', 'OpticsCore', 'SolidStateCore', 'ThermodynamicsCore')
	foreach ($name in $names) {
		$repo = Join-Path $repos $name
		New-Item -ItemType Directory -Path $repo | Out-Null
		& git -C $repo init --quiet
		if ($LASTEXITCODE -ne 0) { throw "Git init failed: $name" }
		[IO.File]::WriteAllText((Join-Path $repo 'tracked.txt'), $name)
		& git -C $repo add tracked.txt
		if ($LASTEXITCODE -ne 0) { throw "Git add failed: $name" }
		& git -C $repo -c user.name=Fixture -c user.email=fixture@example.invalid commit --quiet -m fixture
		if ($LASTEXITCODE -ne 0) { throw "Git commit failed: $name" }
	}
	$project = Join-Path $repos 'PhysicsEngineCore'
	[IO.File]::WriteAllText((Join-Path $repos 'QuantumCore\untracked.txt'), 'dirty')
	$build = Join-Path $temp 'build'
	$package = Join-Path $temp 'package'
	New-Item -ItemType Directory -Path $build, (Join-Path $package 'include'), (Join-Path $package 'lib') -Force | Out-Null
	[IO.File]::WriteAllLines((Join-Path $build 'CMakeCache.txt'), [string[]]@('CMAKE_BUILD_TYPE:STRING=Release', 'CMAKE_GENERATOR:INTERNAL=Ninja', 'CMAKE_CXX_COMPILER:FILEPATH=cl.exe'))
	[IO.File]::WriteAllText((Join-Path $package 'include\api.h'), 'header')
	[IO.File]::WriteAllText((Join-Path $package 'lib\engine.lib'), 'library')
	$symbols = Join-Path $temp 'symbols.txt'
	[IO.File]::WriteAllText($symbols, "pe_version`n")
	$output = Join-Path $temp 'evidence.json'
	& $script -ProjectRoot $project -ReposRoot $repos -BuildRoot $build -PackageRoot $package -SymbolListPath $symbols -OutputPath $output
	$evidence = Get-Content -Raw -LiteralPath $output | ConvertFrom-Json
	$commit = (& git -C $project rev-parse HEAD).Trim()
	if ($LASTEXITCODE -ne 0 -or $evidence.repositories.PhysicsEngineCore.commit -cne $commit -or
		$evidence.repositories.PhysicsEngineCore.dirty -or -not $evidence.repositories.QuantumCore.dirty) {
		throw 'Repository evidence did not match the fixture.'
	}
	if ($evidence.cmake_cache.CMAKE_BUILD_TYPE -cne 'Release' -or $evidence.package_files.Count -ne 2 -or
		$evidence.symbol_list.sha256 -cne (Get-FileHash -LiteralPath $symbols -Algorithm SHA256).Hash.ToLowerInvariant()) {
		throw 'Build metadata, symbol hash, or package file count did not match.'
	}
	foreach ($entry in $evidence.package_files) {
		$path = Join-Path $package $entry.path
		if ($entry.sha256 -cne (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()) { throw "Incorrect file hash: $($entry.path)" }
	}
	$untracked = Join-Path $project 'new-source.txt'
	[IO.File]::WriteAllText($untracked, 'untracked source')
	$untrackedOutput = Join-Path $temp 'untracked-evidence.json'
	& $script -ProjectRoot $project -ReposRoot $repos -BuildRoot $build -PackageRoot $package -SymbolListPath $symbols -OutputPath $untrackedOutput
	$untrackedEvidence = Get-Content -Raw -LiteralPath $untrackedOutput | ConvertFrom-Json
	if (-not $untrackedEvidence.repositories.PhysicsEngineCore.dirty) { throw 'Untracked source change was not recorded.' }
	Remove-Item -LiteralPath $untracked
	[IO.File]::WriteAllText((Join-Path $project 'tracked.txt'), 'modified source')
	$trackedOutput = Join-Path $temp 'tracked-evidence.json'
	& $script -ProjectRoot $project -ReposRoot $repos -BuildRoot $build -PackageRoot $package -SymbolListPath $symbols -OutputPath $trackedOutput
	$trackedEvidence = Get-Content -Raw -LiteralPath $trackedOutput | ConvertFrom-Json
	if (-not $trackedEvidence.repositories.PhysicsEngineCore.dirty) { throw 'Tracked source change was not recorded.' }
	$original = [IO.File]::ReadAllText($output)
	$rejected = $false
	try { & $script -ProjectRoot $project -ReposRoot $repos -BuildRoot $build -PackageRoot $package -SymbolListPath $symbols -OutputPath $output } catch {
		if ($_.Exception.Message -cnotmatch 'Refusing to overwrite evidence') { throw }
		$rejected = $true
	}
	if (-not $rejected -or [IO.File]::ReadAllText($output) -cne $original) { throw 'Existing evidence was not protected.' }
	$rejected = $false
	try { & $script -ProjectRoot $project -ReposRoot $repos -BuildRoot $build -PackageRoot $package -SymbolListPath $symbols -OutputPath (Join-Path $package 'evidence.json') } catch {
		if ($_.Exception.Message -cnotmatch 'outside the package') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'Evidence output inside the package was accepted.' }
	Remove-Item -LiteralPath (Join-Path $repos 'OpticsCore') -Recurse -Force
	$missingOutput = Join-Path $temp 'missing.json'
	$rejected = $false
	try { & $script -ProjectRoot $project -ReposRoot $repos -BuildRoot $build -PackageRoot $package -SymbolListPath $symbols -OutputPath $missingOutput } catch { $rejected = $true }
	if (-not $rejected -or (Test-Path -LiteralPath $missingOutput)) { throw 'Missing repository did not fail safely.' }
	Write-Output 'Compatibility evidence fixtures passed.'
} finally {
	Remove-Item -LiteralPath $temp -Recurse -Force
}
