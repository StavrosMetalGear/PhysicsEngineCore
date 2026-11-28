Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script = Join-Path $root 'scripts\check-pinned-revision.ps1'
$temp = Join-Path $root ('out\pinned-revision-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
	& git -C $temp init --quiet
	if ($LASTEXITCODE -ne 0) { throw 'Git init failed.' }
	[IO.File]::WriteAllText((Join-Path $temp 'tracked.txt'), 'fixture')
	& git -C $temp add tracked.txt
	if ($LASTEXITCODE -ne 0) { throw 'Git add failed.' }
	& git -C $temp -c user.name=Fixture -c user.email=fixture@example.invalid commit --quiet -m fixture
	if ($LASTEXITCODE -ne 0) { throw 'Git commit failed.' }
	$commit = (& git -C $temp rev-parse HEAD).Trim()
	if ($LASTEXITCODE -ne 0) { throw 'Could not obtain fixture commit.' }
	& $script -RepositoryPath $temp -ExpectedCommit $commit
	foreach ($case in @(
		@{ Path = $temp; Commit = ('0' * $commit.Length); Message = 'Pinned revision mismatch' },
		@{ Path = $temp; Commit = $commit.Substring(0, 7); Message = 'full lowercase Git commit ID' }
	)) {
		$rejected = $false
		try { & $script -RepositoryPath $case.Path -ExpectedCommit $case.Commit } catch {
			if ($_.Exception.Message -cnotmatch $case.Message) { throw }
			$rejected = $true
		}
		if (-not $rejected) { throw "Invalid pin accepted: $($case.Commit)" }
	}
	$subdirectory = Join-Path $temp 'nested'
	New-Item -ItemType Directory -Path $subdirectory | Out-Null
	$rejected = $false
	try { & $script -RepositoryPath $subdirectory -ExpectedCommit $commit } catch {
		if ($_.Exception.Message -cnotmatch 'Not a repository root') { throw }
		$rejected = $true
	}
	if (-not $rejected) { throw 'Nested directory was accepted as a repository root.' }
	Write-Output 'Pinned revision fixtures passed.'
} finally {
	Remove-Item -LiteralPath $temp -Recurse -Force
}
