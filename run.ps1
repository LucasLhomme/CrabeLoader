# Creates the virtual environment, installs the dependencies, then starts Creator.
# -SetupOnly: install only (used by Launch_Creator.bat on first launch).
param([switch]$SetupOnly)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if (-not (Test-Path ".venv")) {
    Write-Host "Creating the virtual environment..." -ForegroundColor Cyan
    python -m venv .venv
    if ($LASTEXITCODE -ne 0) { Write-Host "Python 3.12+ not found (install it with 'Add python.exe to PATH')." -ForegroundColor Red; exit 1 }
}

. .venv\Scripts\Activate.ps1

Write-Host "Checking dependencies..." -ForegroundColor Cyan
pip install -q -r requirements.txt
if ($LASTEXITCODE -ne 0) { Write-Host "Dependency installation failed." -ForegroundColor Red; exit 1 }

if ($SetupOnly) { exit 0 }
Write-Host "Starting Disney Infinity Creator..." -ForegroundColor Cyan
python src/main.py
