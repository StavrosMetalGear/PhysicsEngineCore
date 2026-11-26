[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$ReposRoot,
	[Parameter(Mandatory = $true)]
	[string]$TestRoot
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
Write-Output "Inventory comparison fixtures passed. Fixture retained at $root"
