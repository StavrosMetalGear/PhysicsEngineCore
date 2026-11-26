[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$LibraryPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script = Join-Path $root 'scripts\inspect-msvc-symbols.ps1'
$inventory = Join-Path $root 'PHASE10_API_INVENTORY.md'
$temp = Join-Path $root ("out\symbol-inspection-test-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null

try {
	$listPath = Join-Path $temp 'symbols.txt'
	& $script -LibraryPath $LibraryPath -SymbolListPath $listPath
	$names = @(Get-Content -LiteralPath $listPath)
	if ('pe_version' -cnotin $names -or 'qp_create' -cnotin $names) {
		throw 'The symbol list is missing expected entries.'
	}

	$contents = Get-Content -Raw -LiteralPath $inventory
	$replacement = $contents -creplace '(?m)^(- Declarations: [^\r\n]*?)\bqp_create\b', '${1}qp_phase11_missing_symbol'
	if ($replacement -ceq $contents) { throw 'Could not create the negative-test inventory.' }
	$badInventory = Join-Path $temp 'missing-symbol.md'
	[IO.File]::WriteAllText($badInventory, $replacement)
	$rejected = $false
	$failedListPath = Join-Path $temp 'failed-symbols.txt'
	try {
		& $script -LibraryPath $LibraryPath -InventoryPath $badInventory -SymbolListPath $failedListPath
	} catch {
		if ($_.Exception.Message -cnotmatch 'qp_phase11_missing_symbol') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'A missing symbol was not detected.' }
	if (Test-Path -LiteralPath $failedListPath) { throw 'A failed verification wrote a symbol list.' }

	[IO.File]::WriteAllText($failedListPath, 'existing evidence')
	$rejected = $false
	try {
		& $script -LibraryPath $LibraryPath -InventoryPath $badInventory -SymbolListPath $failedListPath
	} catch {
		if ($_.Exception.Message -cnotmatch 'qp_phase11_missing_symbol') { throw }
		$rejected = $true
	}
	if (-not $rejected -or [IO.File]::ReadAllText($failedListPath) -cne 'existing evidence') {
		throw 'A failed verification modified an existing symbol list.'
	}

	$rejected = $false
	try {
		& $script -LibraryPath (Join-Path $temp 'missing.lib')
	} catch {
		$rejected = $true
	}
	if (-not $rejected) { throw 'A missing library was not detected.' }
	Write-Output 'Static-library symbol inspection checks passed.'
} finally {
	Remove-Item -LiteralPath $temp -Recurse -Force
}
