Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script = Join-Path $root 'scripts\compare-msvc-c-symbols.ps1'
$temp = Join-Path $root ("out\symbol-comparison-test-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null

try {
	$baseline = Join-Path $temp 'baseline.txt'
	$current = Join-Path $temp 'current.txt'
	$names = [string[]]@('pe_version', 'qp_create', 'qp_destroy', 'cp_create', 'em_create',
		'fluid_reynolds_number', 'np_create', 'op_create', 'ss_lattice_spacing', 'td_create', '?CppInternal@@')
	[IO.File]::WriteAllLines($baseline, $names)
	[IO.File]::WriteAllLines($current, $names)
	$result = @(& $script -BaselinePath $baseline -CurrentPath $current) -join "`n"
	if ($result -cnotmatch 'added 0, removed 0') { throw 'Identical lists were not recognized.' }

	[IO.File]::WriteAllLines($current, [string[]]@($names | Where-Object { $_ -cne '?CppInternal@@' }))
	$result = @(& $script -BaselinePath $baseline -CurrentPath $current) -join "`n"
	if ($result -cnotmatch 'added 0, removed 0') { throw 'An unrelated C++ symbol affected the comparison.' }

	[IO.File]::WriteAllLines($current, [string[]]($names + 'qp_new_api'))
	$result = @(& $script -BaselinePath $baseline -CurrentPath $current) -join "`n"
	if ($result -cnotmatch 'Added: qp_new_api' -or $result -cnotmatch 'added 1, removed 0') {
		throw 'An added symbol was not reported.'
	}

	[IO.File]::WriteAllLines($current, [string[]]@($names | Where-Object { $_ -cne 'qp_destroy' }))
	$rejected = $false
	try { & $script -BaselinePath $baseline -CurrentPath $current } catch {
		if ($_.Exception.Message -cnotmatch 'Removed candidate C names: qp_destroy') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'A removed symbol was not rejected.' }

	[IO.File]::WriteAllLines($current, [string[]]@($names | Where-Object { $_ -cne 'em_create' }))
	$rejected = $false
	try { & $script -BaselinePath $baseline -CurrentPath $current } catch {
		if ($_.Exception.Message -cnotmatch 'No em_ symbols') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'A list missing a symbol family was not rejected.' }

	[IO.File]::WriteAllLines($current, [string[]]($names + 'qp_create'))
	$rejected = $false
	try { & $script -BaselinePath $baseline -CurrentPath $current } catch {
		if ($_.Exception.Message -cnotmatch 'Duplicate symbol') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'A duplicate symbol was not rejected.' }

	[IO.File]::WriteAllLines($current, [string[]]($names + 'invalid symbol'))
	$rejected = $false
	try { & $script -BaselinePath $baseline -CurrentPath $current } catch {
		if ($_.Exception.Message -cnotmatch 'Invalid symbol entry') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'An invalid symbol entry was not rejected.' }
	Write-Output 'Static-library C symbol comparison checks passed.'
} finally {
	Remove-Item -LiteralPath $temp -Recurse -Force
}
