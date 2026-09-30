param (
    [string]$GameDir = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $scriptDir) { $scriptDir = $PSScriptRoot }
$projectRoot = if ((Split-Path -Leaf $scriptDir) -eq "tests") { Split-Path -Parent $scriptDir } else { $scriptDir }
Set-Location $projectRoot

# ==============================================================================
# Game Installation Directory Configuration:
# Auto-detects Disney Infinity 3.0 across Steam registry and common install paths.
# To override on your machine, you can:
#   1. Provide -GameDir parameter: .\test_game.ps1 -GameDir "C:\YourPath\..."
#   2. Set the environment variable: $env:CRABE_GAME_DIR = "C:\YourPath\..."
# ==============================================================================
function Resolve-GameDirectory {
    param ([string]$ExplicitPath)

    if ($ExplicitPath -and (Test-Path (Join-Path $ExplicitPath "DisneyInfinity3.exe"))) {
        return (Resolve-Path $ExplicitPath).Path
    }

    if ($env:CRABE_GAME_DIR -and (Test-Path (Join-Path $env:CRABE_GAME_DIR "DisneyInfinity3.exe"))) {
        return (Resolve-Path $env:CRABE_GAME_DIR).Path
    }

    $candidates = @(
        "D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition",
        "C:\Program Files (x86)\Steam\steamapps\common\Disney Infinity 3.0 Gold Edition",
        "C:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition",
        "E:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition",
        "F:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition"
    )

    $regKeys = @(
        "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 541670",
        "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 541670"
    )
    foreach ($rk in $regKeys) {
        $loc = (Get-ItemProperty -Path $rk -ErrorAction SilentlyContinue).InstallLocation
        if ($loc) { $candidates += $loc }
    }

    foreach ($c in $candidates) {
        if ($c -and (Test-Path (Join-Path $c "DisneyInfinity3.exe"))) {
            return (Resolve-Path $c).Path
        }
    }

    Write-Error "DisneyInfinity3.exe not found. Specify -GameDir or set `$env:CRABE_GAME_DIR."
    exit 1
}

$resolvedGameDir = Resolve-GameDirectory -ExplicitPath $GameDir
$gameExe = Join-Path $resolvedGameDir "DisneyInfinity3.exe"
$dllCandidates = @(
    (Join-Path $projectRoot "build\Release\bink2w32.dll"),
    (Join-Path $projectRoot "Release\bink2w32.dll"),
    (Join-Path $projectRoot "build\vs2022-dll\Release\bink2w32.dll")
)

Write-Host "=== 1. Closing Game (if running) ===" -ForegroundColor Cyan
Stop-Process -Name "DisneyInfinity3" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 800

Write-Host "=== 2. Building DLL (Release Win32) ===" -ForegroundColor Cyan
cmake --build (Join-Path $projectRoot "build") --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit $LASTEXITCODE
}

Write-Host "=== 3. Deploying artifacts ===" -ForegroundColor Cyan
$origDll = Join-Path $resolvedGameDir "bink2w32_orig.dll"
$targetDll = Join-Path $resolvedGameDir "bink2w32.dll"
if (!(Test-Path $origDll) -and (Test-Path $targetDll)) {
    Copy-Item -Path $targetDll -Destination $origDll -Force
}

$dllSrc = $dllCandidates | Where-Object { Test-Path $_ } | Sort-Object { (Get-Item $_).LastWriteTime } -Descending | Select-Object -First 1
Copy-Item -Path $dllSrc -Destination $targetDll -Force
Write-Host "[OK] bink2w32.dll -> $resolvedGameDir" -ForegroundColor Green

# Clean legacy API folder if present (Lua API is compiled into the DLL)
$apiDir = Join-Path $resolvedGameDir "api"
if (Test-Path $apiDir) {
    Remove-Item -Path $apiDir -Recurse -Force -ErrorAction SilentlyContinue
}

# Deploy to mods/ directory
$modsDir = Join-Path $resolvedGameDir "mods"
if (!(Test-Path $modsDir)) { New-Item -ItemType Directory -Force -Path $modsDir | Out-Null }
if (Test-Path (Join-Path $projectRoot "mods")) {
    Copy-Item -Path (Join-Path $projectRoot "mods\*.lua") -Destination $modsDir -Force -ErrorAction SilentlyContinue
    Get-ChildItem -Path (Join-Path $projectRoot "mods") -Directory |
        Where-Object { $_.Name -notmatch '^[._]' } |
        ForEach-Object {
            $modDest = Join-Path $modsDir $_.Name
            if (!(Test-Path $modDest)) { New-Item -ItemType Directory -Force -Path $modDest | Out-Null }
            Copy-Item -Path (Join-Path $_.FullName '*') -Destination $modDest -Recurse -Force
        }
}
$menuMod = Join-Path $projectRoot "..\CrabeMenu\mods\crabemenu.lua"
if (Test-Path $menuMod) {
    Copy-Item -Path $menuMod -Destination (Join-Path $modsDir "crabemenu.lua") -Force
}

# Synchronize skilltrees directory if present
$skilltreesDir = Join-Path $projectRoot "skilltrees"
if (Test-Path $skilltreesDir) {
    $dest = Join-Path $resolvedGameDir "skilltrees"
    if (!(Test-Path $dest)) { New-Item -ItemType Directory -Force -Path $dest | Out-Null }
    Copy-Item -Path "$skilltreesDir\*" -Destination $dest -Recurse -Force
}

Write-Host "=== 4. Launching game ===" -ForegroundColor Green
if (Get-Process -Name "steam" -ErrorAction SilentlyContinue) {
    Start-Process "steam://rungameid/541670"
} else {
    Start-Process -FilePath $gameExe -WorkingDirectory $resolvedGameDir
}
Write-Host "Game launched successfully!" -ForegroundColor Green
