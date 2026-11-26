[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$LibraryPath,
	[string]$InventoryPath = (Join-Path $PSScriptRoot '..\PHASE10_API_INVENTORY.md'),
	[string]$SymbolListPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$library = (Resolve-Path -LiteralPath $LibraryPath).Path
$inventory = (Resolve-Path -LiteralPath $InventoryPath).Path
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
$declarationLines = @(Get-Content -LiteralPath $inventory | Where-Object { $_ -match '^- Declarations: ' })
if ($declarationLines.Count -ne 8) {
	throw "Expected eight module declaration lists in the inventory; found $($declarationLines.Count)."
}
foreach ($line in $declarationLines) {
	foreach ($name in (($line -replace '^- Declarations: ', '') -split ', ')) {
		if ($name -cnotmatch '^(qp|cp|em|fluid|np|op|ss|td)_[A-Za-z0-9_]+$') {
			throw "Invalid declaration in inventory: $name"
		}
		[void]$required.Add($name)
	}
}
$moduleCount = $required.Count
foreach ($header in @('PhysicsMetadata.h', 'PhysicsAsync.h', 'PhysicsCoupling.h')) {
	$text = Get-Content -Raw -LiteralPath (Join-Path $projectRoot $header)
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
	$sortedSymbols = [string[]]@($symbols)
	[Array]::Sort($sortedSymbols, [StringComparer]::Ordinal)
	[IO.File]::WriteAllLines($listPath, $sortedSymbols)
}

Write-Output "Verified $($required.Count) C symbols ($moduleCount module, $($required.Count - $moduleCount) engine); archive contains $symbolRows public symbol entries."
