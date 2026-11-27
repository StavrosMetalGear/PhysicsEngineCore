$modules = @(
	@{ Name = 'QuantumCore'; Prefix = 'quantum'; Header = 'quantum_c_api.h'; Source = 'quantum_c_api.cpp' },
	@{ Name = 'ClassicalMechanicsCore'; Prefix = 'classical'; Header = 'classical_c_api.h'; Source = 'classical_c_api.cpp' },
	@{ Name = 'ElectrodynamicsCore'; Prefix = 'em'; Header = 'em_c_api.h'; Source = 'em_c_api.cpp' },
	@{ Name = 'FluidDynamicsCore'; Prefix = 'fluid'; Header = 'fluid_c_api.h'; Source = 'fluid_c_api.cpp' },
	@{ Name = 'NuclearCore'; Prefix = 'nuclear'; Header = 'nuclear_c_api.h'; Source = 'nuclear_c_api.cpp' },
	@{ Name = 'OpticsCore'; Prefix = 'optics'; Header = 'optics_c_api.h'; Source = 'optics_c_api.cpp' },
	@{ Name = 'SolidStateCore'; Prefix = 'solidstate'; Header = 'solidstate_c_api.h'; Source = 'solidstate_c_api.cpp' },
	@{ Name = 'ThermodynamicsCore'; Prefix = 'thermo'; Header = 'thermo_c_api.h'; Source = 'thermo_c_api.cpp' }
)

function Remove-CppCommentsAndStrings([string]$Text) {
	$Text = [regex]::Replace($Text, '/\*.*?\*/', '', [Text.RegularExpressions.RegexOptions]::Singleline)
	$Text = [regex]::Replace($Text, '//[^\r\n]*', '')
	$Text = [regex]::Replace($Text, '"(?:\\.|[^"\\])*"', '""')
	return $Text
}

function Get-FunctionNames([string]$Path, [ValidateSet('Declaration', 'Definition')][string]$Kind) {
	$raw = Get-Content -Raw -LiteralPath $Path
	$text = Remove-CppCommentsAndStrings $raw
	$pattern = '(?ms)(?:^|[;{}])\s*(?:(?:extern\s+"C"|inline|static|const|constexpr|[A-Z_][A-Z0-9_]*_API)\s+)*[A-Za-z_][\w:\s\*&<>]*?\b([A-Za-z_]\w*)\s*\((.*?)\)\s*(?:const\s*)?(?<terminator>;|\{)'
	$names = [System.Collections.Generic.List[string]]::new()
	foreach ($match in [regex]::Matches($text, $pattern)) {
		$terminator = $match.Groups['terminator'].Value
		if (($Kind -eq 'Declaration' -and $terminator -ne ';') -or
			($Kind -eq 'Definition' -and $terminator -ne '{')) {
			continue
		}

		$name = $match.Groups[1].Value
		if ($name -notmatch '^(qp|cp|em|fluid|np|op|ss|td)_') {
			continue
		}
		if (-not $names.Contains($name)) {
			$names.Add($name)
		}
	}
	return @($names | Sort-Object)
}
