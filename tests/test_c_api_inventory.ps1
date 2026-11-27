[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$ReposRoot,
	[Parameter(Mandatory = $true)]
	[string]$TestRoot,
	[Parameter(Mandatory = $true)]
	[string]$HeadersRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $TestRoot) { throw "Refusing to overwrite an existing test directory: $TestRoot" }
$null = New-Item -ItemType Directory -Path $TestRoot
$root = (Resolve-Path -LiteralPath $TestRoot).Path
$project = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script = Join-Path $project 'scripts\check-c-api-inventory.ps1'
$inventory = Join-Path $project 'PHASE10_API_INVENTORY.md'
& $script -ReposRoot $ReposRoot -InventoryPath $inventory -ScratchPath (Join-Path $root 'matching')
& $script -ReposRoot $ReposRoot -HeadersRoot $HeadersRoot -InventoryPath $inventory -ScratchPath (Join-Path $root 'installed-matching')

$text = [IO.File]::ReadAllText($inventory)
$changed = $text -creplace '(?m)^(- Declarations: [^\r\n]*?)\bqp_create\b', '${1}qp_phase11_missing_symbol'
if ($changed -ceq $text) { throw 'Could not create the negative inventory fixture.' }
$fixture = Join-Path $root 'changed.md'
[IO.File]::WriteAllText($fixture, $changed)
$rejected = $false
try {
	& $script -ReposRoot $ReposRoot -InventoryPath $fixture -ScratchPath (Join-Path $root 'changed')
} catch {
	if ($_.Exception.Message -cnotmatch 'QuantumCore removed: qp_phase11_missing_symbol' -or
		$_.Exception.Message -cnotmatch 'QuantumCore added: qp_create') { throw }
	$rejected = $true
}
if (-not $rejected) { throw 'Changed declaration names were not detected.' }

$headerFixture = Join-Path $root 'installed-headers'
New-Item -ItemType Directory -Path $headerFixture | Out-Null
foreach ($header in @('quantum_c_api.h', 'classical_c_api.h', 'em_c_api.h', 'fluid_c_api.h',
	'nuclear_c_api.h', 'optics_c_api.h', 'solidstate_c_api.h', 'thermo_c_api.h')) {
	Copy-Item -LiteralPath (Join-Path $HeadersRoot $header) -Destination $headerFixture
}
$quantum = Join-Path $headerFixture 'quantum_c_api.h'
$quantumText = [IO.File]::ReadAllText($quantum)
$modifiedQuantum = $quantumText -creplace '\bqp_create\b', 'qp_phase11_changed_create'
if ($modifiedQuantum -ceq $quantumText) { throw 'Could not create the changed installed-header fixture.' }
[IO.File]::WriteAllText($quantum, $modifiedQuantum)
$rejected = $false
try {
	& $script -ReposRoot $ReposRoot -HeadersRoot $headerFixture -InventoryPath $inventory -ScratchPath (Join-Path $root 'installed-changed')
} catch {
	if ($_.Exception.Message -cnotmatch 'QuantumCore removed: qp_create' -or
		$_.Exception.Message -cnotmatch 'QuantumCore added: qp_phase11_changed_create') { throw }
	$rejected = $true
}
if (-not $rejected) { throw 'Changed installed declaration names were not detected.' }

Remove-Item -LiteralPath (Join-Path $headerFixture 'fluid_c_api.h')
$rejected = $false
try {
	& $script -ReposRoot $ReposRoot -HeadersRoot $headerFixture -InventoryPath $inventory -ScratchPath (Join-Path $root 'installed-missing')
} catch {
	if ($_.Exception.Message -cnotmatch 'Missing API header for FluidDynamicsCore') { throw }
	$rejected = $true
}
if (-not $rejected) { throw 'A missing installed Fluid header was not detected.' }
$rejected = $false
try {
	& $script -ReposRoot $ReposRoot -HeadersRoot '' -InventoryPath $inventory -ScratchPath (Join-Path $root 'installed-empty')
} catch {
	if ($_.Exception.Message -cnotmatch 'Invalid API header directory') { throw }
	$rejected = $true
}
if (-not $rejected) { throw 'An empty installed header directory was not rejected.' }
Write-Output "Inventory comparison fixtures passed. Fixture retained at $root"
