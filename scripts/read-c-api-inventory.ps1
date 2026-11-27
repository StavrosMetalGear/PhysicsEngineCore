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
