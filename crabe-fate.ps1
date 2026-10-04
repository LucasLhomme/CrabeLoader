<#
.SYNOPSIS
    Compiles and executes the CRABE-FATE regression and golden reference test suite.

.DESCRIPTION
    Builds the `crabe_fate` CMake target (Release by default) and runs the runner
    against the test cases in tests/fate/samples/, verifying outputs with tests/fate/ref/.

.PARAMETER Gen
    Regenerates the reference golden (.ref) files.

.PARAMETER Config
    Build configuration (Release, Debug, RelWithDebInfo). Default: Release.

.PARAMETER BuildOnly
    Compiles the crabe_fate target without running the test suite.

.EXAMPLE
    .\crabe-fate.ps1
    Runs all FATE tests.

.EXAMPLE
    .\crabe-fate.ps1 -Gen
    Regenerates golden reference files.
#>

[CmdletBinding()]
param (
    [Alias("g")]
    [switch]$Gen,

    [string]$Config = "Release",

    [switch]$BuildOnly
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $scriptDir) { $scriptDir = $PSScriptRoot }
$projectRoot = if ((Split-Path -Leaf $scriptDir) -eq "tests") { Split-Path -Parent $scriptDir } else { $scriptDir }
Set-Location $projectRoot

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "             CRABE-FATE Regression Test Suite              " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Configuration : $Config" -ForegroundColor Gray
Write-Host "Generate Refs : $(if ($Gen) { 'YES' } else { 'NO' })" -ForegroundColor Gray
Write-Host ""

# 1. Compilation
Write-Host "[1/2] Compiling crabe_fate ($Config)..." -ForegroundColor Yellow
$buildStartTime = [System.Diagnostics.Stopwatch]::StartNew()

cmake --build (Join-Path $projectRoot "build") --target crabe_fate --config $Config
if ($LASTEXITCODE -ne 0) {
    Write-Host "`n[ERROR] Build failed with exit code $LASTEXITCODE." -ForegroundColor Red
    exit $LASTEXITCODE
}

$buildStartTime.Stop()
Write-Host "[OK] Build completed in $([math]::Round($buildStartTime.Elapsed.TotalSeconds, 2))s.`n" -ForegroundColor Green

if ($BuildOnly) {
    Write-Host "[INFO] Build-only requested. Exiting." -ForegroundColor Yellow
    exit 0
}

# 2. Locate crabe_fate executable
$candidates = @(
    (Join-Path $projectRoot "build\$Config\crabe_fate.exe"),
    (Join-Path $projectRoot "build\crabe_fate.exe"),
    (Join-Path $projectRoot "build\Release\crabe_fate.exe")
)

$exePath = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $exePath) {
    Write-Host "[ERROR] Could not locate crabe_fate.exe in build directories." -ForegroundColor Red
    exit 1
}

# 3. Execution
Write-Host "[2/2] Running CRABE-FATE ($exePath)..." -ForegroundColor Yellow
Write-Host ""

$fateArgs = @()
if ($Gen) {
    $fateArgs += "--gen"
}

& $exePath $fateArgs
$testExitCode = $LASTEXITCODE

Write-Host ""
if ($testExitCode -eq 0) {
    Write-Host "============================================================" -ForegroundColor Green
    Write-Host "           CRABE-FATE PASSED ALL REGRESSION TESTS           " -ForegroundColor Green
    Write-Host "============================================================" -ForegroundColor Green
} else {
    Write-Host "============================================================" -ForegroundColor Red
    Write-Host "           CRABE-FATE FAILED (Exit Code: $testExitCode)      " -ForegroundColor Red
    Write-Host "============================================================" -ForegroundColor Red
}

exit $testExitCode
