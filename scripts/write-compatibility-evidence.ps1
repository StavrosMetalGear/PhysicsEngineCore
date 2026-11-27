[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)][string]$ProjectRoot,
	[Parameter(Mandatory = $true)][string]$ReposRoot,
	[Parameter(Mandatory = $true)][string]$BuildRoot,
	[Parameter(Mandatory = $true)][string]$PackageRoot,
	[Parameter(Mandatory = $true)][string]$SymbolListPath,
	[Parameter(Mandatory = $true)][string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-RepositoryEvidence([string]$Path) {
	$directory = Get-Item -LiteralPath $Path
	if (-not $directory.PSIsContainer) { throw "Not a repository directory: $Path" }
	$top = @(& git -C $directory.FullName rev-parse --show-toplevel)
	if ($LASTEXITCODE -ne 0 -or $top.Count -ne 1 -or
		-not [StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($top[0]).TrimEnd('\', '/'), $directory.FullName.TrimEnd('\', '/'))) {
		throw "Not a repository root: $Path"
	}
	$commit = @(& git -C $directory.FullName rev-parse --verify 'HEAD^{commit}')
	if ($LASTEXITCODE -ne 0 -or $commit.Count -ne 1 -or $commit[0] -cnotmatch '^[0-9a-f]{40,64}$') { throw "Could not read repository commit: $Path" }
	$status = @(& git -C $directory.FullName status --porcelain --untracked-files=normal)
	if ($LASTEXITCODE -ne 0) { throw "Could not read repository status: $Path" }
	return [ordered]@{ commit = $commit[0]; dirty = ($status.Count -gt 0) }
}

$package = Get-Item -LiteralPath $PackageRoot
if (-not $package.PSIsContainer -or ($package.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
	throw "Package root must be a non-link directory: $PackageRoot"
}
$symbol = Get-Item -LiteralPath $SymbolListPath
if ($symbol.PSIsContainer -or ($symbol.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $symbol.Length -eq 0) {
	throw "Symbol list must be a nonempty regular file: $SymbolListPath"
}
$output = [IO.Path]::GetFullPath($OutputPath)
if (Test-Path -LiteralPath $output) { throw "Refusing to overwrite evidence: $output" }
$packagePrefix = $package.FullName.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
if ($output.StartsWith($packagePrefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence output must be outside the package.' }
$cache = Join-Path $BuildRoot 'CMakeCache.txt'
$cacheItem = Get-Item -LiteralPath $cache
if ($cacheItem.PSIsContainer -or $cacheItem.Length -eq 0) { throw "Missing or empty CMake cache: $cache" }

$repositories = [ordered]@{}
$repositories['PhysicsEngineCore'] = Get-RepositoryEvidence $ProjectRoot
foreach ($name in @('QuantumCore', 'ClassicalMechanicsCore', 'ElectrodynamicsCore', 'FluidDynamicsCore',
	'NuclearCore', 'OpticsCore', 'SolidStateCore', 'ThermodynamicsCore')) {
	$repositories[$name] = Get-RepositoryEvidence (Join-Path $ReposRoot $name)
}

$buildMetadata = [ordered]@{}
foreach ($line in [IO.File]::ReadAllLines($cacheItem.FullName)) {
	if ($line -cmatch '^(CMAKE_(?:BUILD_TYPE|GENERATOR|GENERATOR_PLATFORM|C_COMPILER|CXX_COMPILER|MSVC_RUNTIME_LIBRARY|C_FLAGS(?:_[A-Z]+)?|CXX_FLAGS(?:_[A-Z]+)?)):[^=]+=(.*)$') {
		$buildMetadata[$Matches[1]] = $Matches[2]
	}
}
foreach ($key in @('CMAKE_BUILD_TYPE', 'CMAKE_GENERATOR', 'CMAKE_CXX_COMPILER')) {
	if (-not $buildMetadata.Contains($key) -or [string]::IsNullOrWhiteSpace($buildMetadata[$key])) { throw "Missing CMake cache metadata: $key" }
}

$files = [System.Collections.Generic.List[object]]::new()
$pending = [System.Collections.Generic.Stack[object]]::new()
$pending.Push([pscustomobject]@{ Directory = $package; Prefix = '' })
while ($pending.Count -gt 0) {
	$entry = $pending.Pop()
	foreach ($item in (Get-ChildItem -LiteralPath $entry.Directory.FullName -Force)) {
		$relative = if ($entry.Prefix) { "$($entry.Prefix)/$($item.Name)" } else { $item.Name }
		if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Links are not supported in a package: $relative" }
		if ($item.PSIsContainer) {
			$pending.Push([pscustomobject]@{ Directory = $item; Prefix = $relative })
		} else {
			$files.Add([pscustomobject]@{ path = $relative; sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant() })
		}
	}
}
if ($files.Count -eq 0) { throw "Package is empty: $PackageRoot" }
$evidence = [ordered]@{
	format = 'physics-compatibility-evidence-v1'
	generated_utc = [DateTime]::UtcNow.ToString('o')
	note = 'Local observations only; build provenance, cleanliness of ignored files, and ABI compatibility are not verified.'
	repositories = $repositories
	cmake_cache = $buildMetadata
	cmake_cache_sha256 = (Get-FileHash -LiteralPath $cacheItem.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
	symbol_list = [ordered]@{ name = $symbol.Name; sha256 = (Get-FileHash -LiteralPath $symbol.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
	package_files = @($files | Sort-Object -Property path)
}
$json = $evidence | ConvertTo-Json -Depth 8
$stream = [IO.File]::Open($output, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
try {
	$writer = [IO.StreamWriter]::new($stream, [Text.UTF8Encoding]::new($false))
	try { $writer.WriteLine($json) } finally { $writer.Dispose() }
} finally { $stream.Dispose() }
Write-Output "Wrote compatibility evidence: $output"
