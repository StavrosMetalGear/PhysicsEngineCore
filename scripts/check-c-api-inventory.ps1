[CmdletBinding()]
param(
	[string]$ReposRoot = (Join-Path $PSScriptRoot '..\..'),
	[string]$InventoryPath = (Join-Path $PSScriptRoot '..\PHASE10_API_INVENTORY.md'),
	[Parameter(Mandatory = $true)]
	[string]$ScratchPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $ScratchPath) { throw "Refusing to overwrite an existing scratch directory: $ScratchPath" }
$null = New-Item -ItemType Directory -Path $ScratchPath
$scratch = (Resolve-Path -LiteralPath $ScratchPath).Path
$generated = Join-Path $scratch 'inventory.md'
& (Join-Path $PSScriptRoot 'audit-c-api.ps1') -ReposRoot $ReposRoot -OutputPath $generated `
	-CategoryOutputPath (Join-Path $scratch 'categories.md') -TestPlanOutputPath (Join-Path $scratch 'test-plan.md')

function Read-Inventory([string]$Path) {
	$sets = @{}
	$current = $null
	foreach ($line in [IO.File]::ReadAllLines((Resolve-Path -LiteralPath $Path).Path)) {
		if ($line -cmatch '^## ([A-Za-z]+Core)$') {
			$current = $Matches[1]
			if ($sets.ContainsKey($current)) { throw "Duplicate module in ${Path}: $current" }
			$sets[$current] = $null
		} elseif ($line -cmatch '^- Declarations: (.+)$') {
			if ($null -eq $current -or $null -ne $sets[$current]) { throw "Unexpected declaration list in $Path" }
			$names = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
			foreach ($name in ($Matches[1] -split ', ')) {
				if ($name -cnotmatch '^(qp|cp|em|fluid|np|op|ss|td)_[A-Za-z0-9_]+$' -or -not $names.Add($name)) {
					throw "Invalid or duplicate declaration in ${Path}: $name"
				}
			}
			$sets[$current] = $names
		}
	}
	$modules = @('QuantumCore', 'ClassicalMechanicsCore', 'ElectrodynamicsCore', 'FluidDynamicsCore',
		'NuclearCore', 'OpticsCore', 'SolidStateCore', 'ThermodynamicsCore')
	if ($sets.Count -ne $modules.Count) { throw "Expected eight module lists in $Path" }
	foreach ($module in $modules) {
		if (-not $sets.ContainsKey($module) -or $null -eq $sets[$module] -or $sets[$module].Count -eq 0) {
			throw "Missing declaration list for $module in $Path"
		}
	}
	return $sets
}

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
