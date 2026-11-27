[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$LibraryPath,
	[string]$EngineHeadersPath = (Join-Path $PSScriptRoot '..')
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
	& $script -LibraryPath $LibraryPath -EngineHeadersPath $EngineHeadersPath -SymbolListPath $listPath
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

	$malformedFixtures = @(
		@{ Name = 'duplicate-module'; Text = $contents -creplace '(?m)^## OpticsCore\s*$', '## QuantumCore'; Expected = 'Duplicate module.*QuantumCore' },
		@{ Name = 'duplicate-name'; Text = $contents -creplace '(?m)^(- Declarations: [^\r\n]*?)\bqp_create\b', '${1}qp_create, qp_create'; Expected = 'Invalid or duplicate declaration.*qp_create' },
		@{ Name = 'missing-list'; Text = $contents -creplace '(?m)^(- Declarations: )(?=op_)[^\r\n]+', '${1}'; Expected = 'Missing declaration list for OpticsCore' }
	)
	foreach ($case in $malformedFixtures) {
		if ($case.Text -ceq $contents) { throw "Could not create fixture $($case.Name)." }
		$fixture = Join-Path $temp "$($case.Name).md"
		[IO.File]::WriteAllText($fixture, $case.Text)
		$rejected = $false
		try {
			& $script -LibraryPath $LibraryPath -InventoryPath $fixture -SymbolListPath $failedListPath
		} catch {
			if ($_.Exception.Message -cnotmatch $case.Expected) { throw }
			$rejected = $true
		}
		if (-not $rejected -or [IO.File]::ReadAllText($failedListPath) -cne 'existing evidence') {
			throw "Malformed inventory $($case.Name) was not safely rejected."
		}
	}

	$headerFixture = Join-Path $temp 'headers'
	New-Item -ItemType Directory -Path $headerFixture | Out-Null
	foreach ($header in @('PhysicsMetadata.h', 'PhysicsAsync.h', 'PhysicsCoupling.h')) {
		Copy-Item -LiteralPath (Join-Path $EngineHeadersPath $header) -Destination $headerFixture
	}
	$metadata = Join-Path $headerFixture 'PhysicsMetadata.h'
	$originalMetadata = [IO.File]::ReadAllText($metadata)
	$rejected = $false
	try {
		& $script -LibraryPath $LibraryPath -EngineHeadersPath $headerFixture -SymbolListPath $metadata
	} catch {
		if ($_.Exception.Message -cnotmatch 'must not overwrite an engine header') { throw }
		$rejected = $true
	}
	if (-not $rejected -or [IO.File]::ReadAllText($metadata) -cne $originalMetadata) {
		throw 'An engine header was not protected from symbol output.'
	}
	[IO.File]::AppendAllText($metadata, "`nPHYSENG_API int pe_phase11_missing_header_symbol(void);`n")
	$rejected = $false
	try {
		& $script -LibraryPath $LibraryPath -EngineHeadersPath $headerFixture
	} catch {
		if ($_.Exception.Message -cnotmatch 'pe_phase11_missing_header_symbol') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'A missing symbol from the selected engine header was not detected.' }

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
