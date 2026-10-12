# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Keep one current full log/JSON pair beside the DLL; automatic timestamped archives and a separate compact format added cleanup and duplicated evidence we rarely revisit.
# Git preserves deliberately shipped JSON reports; save a dated copy manually when a particular failed attempt matters. Failed builds must not label an older DLL as a successful new build. (GPT-6.1-Sol) -->
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('Debug', 'Debug-opt', 'Release', 'Assert-fast', 'Profile-fast', 'Debug-fast', '_Release-fast', 'Final_Release-fast')]
    [string]$Configuration,
    [ValidateSet('BUILD', 'REBUILD', 'CLEAN')]
    [string]$Request = 'BUILD',
    [switch]$IdeBuild
)

$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$target = $Configuration -replace '-fast$', '' -replace '^_', ''
$env:TARGET = $target
$env:SAS_BUILD_REQUEST = $Request
$started = [DateTime]::UtcNow
$timer = [Diagnostics.Stopwatch]::StartNew()
$outputDirectory = Join-Path $PSScriptRoot $target
[void][IO.Directory]::CreateDirectory($outputDirectory)
$logPath = Join-Path $outputDirectory 'AdvCiv.log'
$reportPath = Join-Path $outputDirectory 'CvGameCoreDLL.dll.build.json'
# <!-- custom: MSBuild already owns AdvCiv.log during an IDE build; writing it ourselves would compete with its live logger.
# Terminal builds own the same filename directly, so either entry point produces the current log without another duplicate. (GPT-6.1-Sol) -->
$writer = $null
if (-not $IdeBuild) {
    $writer = New-Object IO.StreamWriter($logPath, $false, (New-Object Text.UTF8Encoding($false)))
    $writer.AutoFlush = $true
}
$warnings = New-Object 'Collections.Generic.HashSet[string]'
$errors = New-Object 'Collections.Generic.HashSet[string]'

function Write-BuildLine([string]$Line) {
    if ($null -ne $writer) { $writer.WriteLine($Line) }
    Write-Output $Line
}

function Get-GitText([string[]]$Arguments) {
    $ErrorActionPreference = 'Continue'
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { return 'UNAVAILABLE' }
    $result = & git @Arguments 2>$null
    if ($LASTEXITCODE -ne 0) { return 'UNAVAILABLE' }
    return ($result -join "`n")
}

function Get-TempState {
    $directory = Join-Path $PSScriptRoot ('temp_files/' + $target)
    $files = @(Get-ChildItem -LiteralPath $directory -Recurse -File -ErrorAction SilentlyContinue)
    return [ordered]@{
        directoryPresent = (Test-Path -LiteralPath $directory)
        objectCount = @($files | Where-Object { $_.Extension -eq '.obj' }).Count
        incompleteMarkerCount = @($files | Where-Object { $_.Extension -eq '.@' }).Count
        pchPresent = (Test-Path -LiteralPath (Join-Path $directory 'CvGameCoreDLL.pch'))
        pdbPresent = (Test-Path -LiteralPath (Join-Path $directory 'CvGameCoreDLL.pdb'))
    }
}

function Get-DllState {
    $path = Join-Path $PSScriptRoot ($target + '/CvGameCoreDLL.dll')
    if (-not (Test-Path -LiteralPath $path)) { return $null }
    $file = Get-Item -LiteralPath $path
    # <!-- custom: The local legacy PowerShell has no Get-FileHash; use .NET SHA-256 so the VC2010 workflow needs no PowerShell upgrade. (GPT-6.1-Sol) -->
    $algorithm = [Security.Cryptography.SHA256]::Create()
    $stream = [IO.File]::OpenRead($path)
    try { $hash = [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '') }
    finally { $stream.Dispose(); $algorithm.Dispose() }
    return [ordered]@{
        sizeBytes = $file.Length
        modifiedUtc = $file.LastWriteTimeUtc.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
        sha256 = $hash
    }
}

# <!-- custom: JSON has no comment syntax; retain introductory copyright/purpose as ordinary metadata so reports remain valid JSON for CI. (GPT-6.1-Sol) -->
$report = [ordered]@{
    copyright = 'AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy); (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS root README.md)'
    purpose = 'Informational build report only: not read or required by Civilization IV or normal gameplay. Supports CI checks for committed DLL updates and investigation of build, runtime or AI issues alongside game logs.'
    configuration = $Configuration
    target = $target
    request = $Request
    startedUtc = $started.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
    branch = Get-GitText -Arguments @('symbolic-ref', '--quiet', '--short', 'HEAD')
    commit = Get-GitText -Arguments @('rev-parse', 'HEAD')
    workingTreeStatus = Get-GitText -Arguments @('status', '--porcelain=v1', '--untracked-files=normal')
    tempBefore = Get-TempState
    dllBefore = Get-DllState
    status = 'RUNNING'
    logFile = [IO.Path]::GetFileName($logPath)
    steps = @()
}
$exitCode = 1
try {
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $reportPath -Encoding UTF8
    Write-BuildLine ('Build report: ' + $reportPath)
    Write-BuildLine ('Started UTC: ' + $report.startedUtc)
    Write-BuildLine ('Configuration: {0}; request: {1}; branch: {2}; commit: {3}' -f $Configuration, $Request, $report.branch, $report.commit)
    Write-BuildLine ('Temporary artifacts before request: ' + ($report.tempBefore | ConvertTo-Json -Compress))
    Write-BuildLine ('Working-tree status: ' + $report.workingTreeStatus)
    if ($Request -eq 'BUILD' -and ($report.tempBefore.objectCount -gt 0 -or $report.tempBefore.pchPresent -or $report.tempBefore.incompleteMarkerCount -gt 0)) {
        Write-BuildLine 'Existing intermediates detected; this wrapper does not clean them. Follow the compilation guide clean-target rule for accepted full builds.'
    }
    $commands = @()
    if ($Request -eq 'REBUILD' -or $Request -eq 'CLEAN') { $commands += 'nmake clean /NOLOGO' }
    if ($Request -ne 'CLEAN') {
        $commands += 'nmake source_list /NOLOGO', 'nmake fastdep /NOLOGO'
        if ($Configuration.EndsWith('-fast')) {
            $commands += 'nmake precompile /NOLOGO', 'bin\jom build /NOLOGO'
        }
        else { $commands += 'nmake dll /NOLOGO' }
    }
    $exitCode = 0
    foreach ($command in $commands) {
        Write-BuildLine ('Command: ' + $command)
        # <!-- custom: Merge native stderr inside cmd so Windows PowerShell does not turn compiler diagnostics into terminating ErrorRecords.
        # Preserve each native exit code and stop at the first failing step; later commands must not hide a failed dependency/compile step. (GPT-6.1-Sol) -->
        & $env:ComSpec /d /s /c ($command + ' 2>&1') | ForEach-Object {
            $line = [string]$_
            Write-BuildLine $line
            if ($line -match '\bwarning\s+[A-Z]+[0-9]+\s*:') { [void]$warnings.Add($line) }
            if ($line -match '\b(?:fatal\s+)?error\s+[A-Z]+[0-9]+\s*:') { [void]$errors.Add($line) }
        }
        $exitCode = $LASTEXITCODE
        $report.steps += [ordered]@{ command = $command; exitCode = $exitCode }
        if ($exitCode -ne 0) { break }
    }
    if ($exitCode -eq 0 -and $Request -ne 'CLEAN' -and -not (Get-DllState)) {
        Write-BuildLine 'Build driver returned success but the target DLL is missing.'
        $exitCode = 1
    }
}
catch {
    Write-BuildLine ('Build reporting/driver failure: ' + $_.Exception.Message)
    $exitCode = 1
}
finally {
    $timer.Stop()
    $report['endedUtc'] = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
    $report['durationMilliseconds'] = $timer.ElapsedMilliseconds
    $report['exitCode'] = $exitCode
    $report.status = if ($exitCode -eq 0) { 'SUCCEEDED' } else { 'FAILED' }
    $report['warningCount'] = $warnings.Count
    $report['errorCount'] = $errors.Count
    $report['recognizedWarningLines'] = @($warnings | Sort-Object)
    $report['recognizedErrorLines'] = @($errors | Sort-Object)
    $report['commitAfter'] = Get-GitText -Arguments @('rev-parse', 'HEAD')
    $report['workingTreeStatusAfter'] = Get-GitText -Arguments @('status', '--porcelain=v1', '--untracked-files=normal')
    $report['tempAfter'] = Get-TempState
    $report['dllAfter'] = Get-DllState
    Write-BuildLine ('Ended UTC: {0}; result: {1}; exit code: {2}; duration milliseconds: {3}' -f $report.endedUtc, $report.status, $exitCode, $report.durationMilliseconds)
    Write-BuildLine ('Distinct coded diagnostic lines: warnings={0}; errors={1}' -f $warnings.Count, $errors.Count)
    Write-BuildLine ('Build output log: ' + $logPath)
    Write-BuildLine ('Build details (JSON): ' + $reportPath)
    if ($null -ne $writer) { $writer.Dispose() }
    [IO.File]::WriteAllText($reportPath, (($report | ConvertTo-Json -Depth 6) + "`n"), (New-Object Text.UTF8Encoding($false)))
}
exit $exitCode
