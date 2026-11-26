[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$BaselineInclude,
	[Parameter(Mandatory = $true)]
	[string]$CurrentInclude
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-HeaderTree([string]$Path) {
	$root = Get-Item -LiteralPath $Path -ErrorAction Stop
	if (-not $root.PSIsContainer -or ($root.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
		throw "Header root must be a non-link directory: $Path"
	}
	$files = [System.Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
	$pending = [System.Collections.Generic.Stack[object]]::new()
	$pending.Push([pscustomobject]@{ Directory = $root; Prefix = '' })
	while ($pending.Count -gt 0) {
		$entry = $pending.Pop()
		foreach ($item in (Get-ChildItem -LiteralPath $entry.Directory.FullName -Force)) {
			$relative = if ($entry.Prefix) { "$($entry.Prefix)/$($item.Name)" } else { $item.Name }
			if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
				throw "Links are not supported in a header tree: $relative"
			}
			if ($item.PSIsContainer) {
				$pending.Push([pscustomobject]@{ Directory = $item; Prefix = $relative })
			} else {
				$files.Add($relative, (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash)
			}
		}
	}
	if ($files.Count -eq 0) { throw "Header tree is empty: $Path" }
	return ,$files
}

$baseline = Read-HeaderTree $BaselineInclude
$current = Read-HeaderTree $CurrentInclude
$added = [System.Collections.Generic.List[string]]::new()
$removed = [System.Collections.Generic.List[string]]::new()
$changed = [System.Collections.Generic.List[string]]::new()
foreach ($path in $baseline.Keys) {
	if (-not $current.ContainsKey($path)) { $removed.Add($path) }
	elseif ($baseline[$path] -cne $current[$path]) { $changed.Add($path) }
}
foreach ($path in $current.Keys) {
	if (-not $baseline.ContainsKey($path)) { $added.Add($path) }
}
$added.Sort([StringComparer]::Ordinal)
$removed.Sort([StringComparer]::Ordinal)
$changed.Sort([StringComparer]::Ordinal)
Write-Output "Header files: baseline $($baseline.Count), current $($current.Count), added $($added.Count), removed $($removed.Count), changed $($changed.Count)."
if ($added.Count) { Write-Output "Added: $($added -join ', ')" }
if ($changed.Count) { Write-Output "Changed: $($changed -join ', ')" }
if ($removed.Count) { throw "Removed header files: $($removed -join ', ')" }
