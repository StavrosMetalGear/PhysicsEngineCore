[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)][string]$RepositoryPath,
	[Parameter(Mandatory = $true)][string]$ExpectedCommit
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($ExpectedCommit -cnotmatch '^[0-9a-f]{40}([0-9a-f]{24})?$') {
	throw "Expected a full lowercase Git commit ID: $ExpectedCommit"
}
$directory = Get-Item -LiteralPath $RepositoryPath
if (-not $directory.PSIsContainer) { throw "Not a repository directory: $RepositoryPath" }
$top = @(& git -C $directory.FullName rev-parse --show-toplevel)
if ($LASTEXITCODE -ne 0 -or $top.Count -ne 1 -or
	-not [StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($top[0]).TrimEnd('\', '/'), $directory.FullName.TrimEnd('\', '/'))) {
	throw "Not a repository root: $RepositoryPath"
}
$actual = @(& git -C $directory.FullName rev-parse --verify 'HEAD^{commit}')
if ($LASTEXITCODE -ne 0 -or $actual.Count -ne 1 -or $actual[0] -cne $ExpectedCommit) {
	throw "Pinned revision mismatch for ${RepositoryPath}: expected $ExpectedCommit; actual $($actual -join ', ')."
}
Write-Output "Verified pinned revision for ${RepositoryPath}: $ExpectedCommit"
