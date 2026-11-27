[CmdletBinding()]
param(
	[string]$ReposRoot = (Join-Path $PSScriptRoot '..\..'),
	[string]$InventoryPath = (Join-Path $PSScriptRoot '..\PHASE10_API_INVENTORY.md'),
	[Parameter(Mandatory = $true)]
	[string]$ScratchPath,
	[string]$HeadersRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $ScratchPath) { throw "Refusing to overwrite an existing scratch directory: $ScratchPath" }
$null = New-Item -ItemType Directory -Path $ScratchPath
$scratch = (Resolve-Path -LiteralPath $ScratchPath).Path
$generated = Join-Path $scratch 'inventory.md'
$headerArgs = @{}
if ($PSBoundParameters.ContainsKey('HeadersRoot')) { $headerArgs.HeadersRoot = $HeadersRoot }
& (Join-Path $PSScriptRoot 'audit-c-api.ps1') -ReposRoot $ReposRoot -OutputPath $generated `
	-CategoryOutputPath (Join-Path $scratch 'categories.md') -TestPlanOutputPath (Join-Path $scratch 'test-plan.md') @headerArgs

. (Join-Path $PSScriptRoot 'read-c-api-inventory.ps1')

$expected = Read-Inventory $InventoryPath
$actual = Read-Inventory $generated
$differences = [System.Collections.Generic.List[string]]::new()
foreach ($module in ($expected.Keys | Sort-Object)) {
	$removed = @($expected[$module] | Where-Object { -not $actual[$module].Contains($_) } | Sort-Object)
	$added = @($actual[$module] | Where-Object { -not $expected[$module].Contains($_) } | Sort-Object)
	if ($removed.Count) { $differences.Add("${module} removed: $($removed -join ', ')") }
	if ($added.Count) { $differences.Add("${module} added: $($added -join ', ')") }
}
if ($differences.Count) { throw "Module declaration inventory drift:`n$($differences -join "`n")" }
Write-Output 'Module declaration names match the current inventory; review parser limitations before relying on this result.'
