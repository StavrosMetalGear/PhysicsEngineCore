[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$LibraryPath,
	[string]$InventoryPath = (Join-Path $PSScriptRoot '..\PHASE10_API_INVENTORY.md'),
	[string]$SymbolListPath,
	[string]$EngineHeadersPath = (Join-Path $PSScriptRoot '..')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$library = (Resolve-Path -LiteralPath $LibraryPath).Path
$inventory = (Resolve-Path -LiteralPath $InventoryPath).Path
$headerDirectory = Get-Item -LiteralPath $EngineHeadersPath
if (-not $headerDirectory.PSIsContainer) { throw "EngineHeadersPath must be a directory: $EngineHeadersPath" }
$dumpbin = (Get-Command dumpbin.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source

$output = @(& $dumpbin /linkermember:1 $library)
if ($LASTEXITCODE -ne 0) {
	throw "dumpbin failed with exit code $LASTEXITCODE"
}

$symbols = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$symbolRows = 0
$expectedRows = -1
$inSymbols = $false
foreach ($line in $output) {
	if ($line -match '^\s*(\d+) public symbols\s*$') {
		if ($expectedRows -ge 0) { throw 'Unexpected multiple symbol tables in dumpbin output.' }
		$expectedRows = [int]$Matches[1]
		$inSymbols = $true
		continue
	}
	if ($inSymbols -and $line -match '^\s*Summary\s*$') { break }
	if ($inSymbols -and $line -match '^\s*[0-9A-Fa-f]+\s+(\S+)\s*$') {
		$symbolRows++
		[void]$symbols.Add($Matches[1])
	}
}
if ($expectedRows -le 0 -or $symbolRows -ne $expectedRows) {
	throw "Could not parse the complete archive symbol table: expected $expectedRows rows, parsed $symbolRows."
}

$required = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
. (Join-Path $PSScriptRoot 'read-c-api-inventory.ps1')
$moduleDeclarations = Read-Inventory $inventory
foreach ($names in $moduleDeclarations.Values) {
	foreach ($name in $names) {
		[void]$required.Add($name)
	}
}
$moduleCount = $required.Count
$headerPaths = [System.Collections.Generic.List[string]]::new()
foreach ($header in @('PhysicsMetadata.h', 'PhysicsAsync.h', 'PhysicsCoupling.h')) {
	$headerPath = (Resolve-Path -LiteralPath (Join-Path $headerDirectory.FullName $header)).Path
	$headerPaths.Add($headerPath)
	$text = Get-Content -Raw -LiteralPath $headerPath
	$matchesInHeader = [regex]::Matches($text, 'PHYSENG_API\s+[^;{}]*?\b(pe_[A-Za-z0-9_]+)\s*\(')
	if ($matchesInHeader.Count -eq 0) { throw "No engine declarations found in $header." }
	foreach ($match in $matchesInHeader) { [void]$required.Add($match.Groups[1].Value) }
}

$missing = @($required | Where-Object { -not $symbols.Contains($_) } | Sort-Object)
if ($missing.Count -ne 0) {
	throw "Missing $($missing.Count) expected C symbols: $($missing -join ', ')"
}

if ($SymbolListPath) {
	$listPath = [IO.Path]::GetFullPath($SymbolListPath)
	if ([StringComparer]::OrdinalIgnoreCase.Equals($listPath, $library) -or
		[StringComparer]::OrdinalIgnoreCase.Equals($listPath, $inventory)) {
		throw 'The symbol list must not overwrite the library or inventory.'
	}
	foreach ($headerPath in $headerPaths) {
		if ([StringComparer]::OrdinalIgnoreCase.Equals($listPath, $headerPath)) {
			throw 'The symbol list must not overwrite an engine header.'
		}
	}
	$sortedSymbols = [string[]]@($symbols)
	[Array]::Sort($sortedSymbols, [StringComparer]::Ordinal)
	[IO.File]::WriteAllLines($listPath, $sortedSymbols)
}

Write-Output "Verified $($required.Count) C symbols ($moduleCount module, $($required.Count - $moduleCount) engine); archive contains $symbolRows public symbol entries."
