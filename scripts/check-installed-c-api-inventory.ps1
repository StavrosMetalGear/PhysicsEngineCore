[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$HeadersRoot,
	[string]$InventoryPath = (Join-Path $PSScriptRoot '..\PHASE10_API_INVENTORY.md')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($HeadersRoot) -or -not (Test-Path -LiteralPath $HeadersRoot -PathType Container)) {
	throw "Invalid API header directory: $HeadersRoot"
}
. (Join-Path $PSScriptRoot 'c-api-audit-common.ps1')
. (Join-Path $PSScriptRoot 'read-c-api-inventory.ps1')
$expected = Read-Inventory $InventoryPath
$differences = [System.Collections.Generic.List[string]]::new()
foreach ($module in $modules) {
	$header = Join-Path $HeadersRoot $module.Header
	if (-not (Test-Path -LiteralPath $header -PathType Leaf)) {
		throw "Missing API header for $($module.Name): $header"
	}
	$actual = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
	foreach ($name in (Get-FunctionNames $header Declaration)) { [void]$actual.Add($name) }
	$removed = @($expected[$module.Name] | Where-Object { -not $actual.Contains($_) } | Sort-Object)
	$added = @($actual | Where-Object { -not $expected[$module.Name].Contains($_) } | Sort-Object)
	if ($removed.Count) { $differences.Add("$($module.Name) removed: $($removed -join ', ')") }
	if ($added.Count) { $differences.Add("$($module.Name) added: $($added -join ', ')") }
}
if ($differences.Count) { throw "Installed module declaration inventory drift:`n$($differences -join "`n")" }
Write-Output 'Installed module declaration names match the inventory; review parser limitations before relying on this result.'
