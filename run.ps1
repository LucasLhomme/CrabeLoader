$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if (-not (Test-Path ".venv")) {
    Write-Host "Creating the virtual environment..." -ForegroundColor Cyan
    python -m venv .venv
}

. .venv\Scripts\Activate.ps1

Write-Host "Checking dependencies..." -ForegroundColor Cyan
pip install -q -r requirements.txt

Write-Host "Starting Disney Infinity Creator..." -ForegroundColor Cyan
python src/main.py
