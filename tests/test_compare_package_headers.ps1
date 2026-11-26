Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script = Join-Path $root 'scripts\compare-package-headers.ps1'
$temp = Join-Path $root ('out\header-comparison-test-' + [guid]::NewGuid().ToString('N'))
$baseline = Join-Path $temp 'baseline'
$current = Join-Path $temp 'current'
New-Item -ItemType Directory -Path (Join-Path $baseline 'physics'), (Join-Path $current 'physics') -Force | Out-Null

try {
	[IO.File]::WriteAllText((Join-Path $baseline 'a.h'), 'same')
	[IO.File]::WriteAllText((Join-Path $current 'a.h'), 'same')
	[IO.File]::WriteAllText((Join-Path $baseline 'physics\b.h'), 'old')
	[IO.File]::WriteAllText((Join-Path $current 'physics\b.h'), 'old')
	$result = @(& $script -BaselineInclude $baseline -CurrentInclude $current) -join "`n"
	if ($result -cnotmatch 'added 0, removed 0, changed 0') { throw 'Identical trees were not recognized.' }

	[IO.File]::WriteAllText((Join-Path $current 'physics\b.h'), 'new')
	[IO.File]::WriteAllText((Join-Path $current 'new.h'), 'addition')
	$result = @(& $script -BaselineInclude $baseline -CurrentInclude $current) -join "`n"
	if ($result -cnotmatch 'added 1, removed 0, changed 1' -or
		$result -cnotmatch 'Added: new\.h' -or $result -cnotmatch 'Changed: physics/b\.h') {
		throw 'Changed or added files were not reported.'
	}

	Remove-Item -LiteralPath (Join-Path $current 'a.h')
	$rejected = $false
	try { & $script -BaselineInclude $baseline -CurrentInclude $current } catch {
		if ($_.Exception.Message -cnotmatch 'Removed header files: a\.h') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'A removed header was not rejected.' }

	$empty = Join-Path $temp 'empty'
	New-Item -ItemType Directory -Path $empty | Out-Null
	$rejected = $false
	try { & $script -BaselineInclude $empty -CurrentInclude $current } catch {
		if ($_.Exception.Message -cnotmatch 'Header tree is empty') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'An empty baseline was not rejected.' }
	Write-Output 'Package header comparison checks passed.'
} finally {
	Remove-Item -LiteralPath $temp -Recurse -Force
}
