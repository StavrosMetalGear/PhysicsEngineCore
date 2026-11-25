[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$BaselinePath,
	[Parameter(Mandatory = $true)]
	[string]$CurrentPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$families = @('pe', 'qp', 'cp', 'em', 'fluid', 'np', 'op', 'ss', 'td')

function Read-CandidateSymbols([string]$Path) {
	$resolved = (Resolve-Path -LiteralPath $Path).Path
	$all = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
	$candidates = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
	foreach ($line in [IO.File]::ReadAllLines($resolved)) {
		if ($line -notmatch '^\S+$') { throw "Invalid symbol entry in $resolved" }
		if (-not $all.Add($line)) { throw "Duplicate symbol in ${resolved}: $line" }
		if ($line -cmatch '^(pe|qp|cp|em|fluid|np|op|ss|td)_[A-Za-z0-9_]+$') {
			[void]$candidates.Add($line)
		}
	}
	foreach ($family in $families) {
		$found = $false
		foreach ($symbol in $candidates) {
			if ($symbol.StartsWith("${family}_", [StringComparison]::Ordinal)) {
				$found = $true
				break
			}
		}
		if (-not $found) { throw "No ${family}_ symbols found in $resolved" }
	}
	return ,$candidates
}

$baseline = Read-CandidateSymbols $BaselinePath
$current = Read-CandidateSymbols $CurrentPath
$removed = [System.Collections.Generic.List[string]]::new()
$added = [System.Collections.Generic.List[string]]::new()
foreach ($symbol in $baseline) {
	if (-not $current.Contains($symbol)) { $removed.Add($symbol) }
}
foreach ($symbol in $current) {
	if (-not $baseline.Contains($symbol)) { $added.Add($symbol) }
}
$removed.Sort([StringComparer]::Ordinal)
$added.Sort([StringComparer]::Ordinal)
Write-Output "Candidate C names: baseline $($baseline.Count), current $($current.Count), added $($added.Count), removed $($removed.Count)."
if ($added.Count) { Write-Output "Added: $($added -join ', ')" }
if ($removed.Count) { throw "Removed candidate C names: $($removed -join ', ')" }
