$ErrorActionPreference = "Stop"

$gameDir = "D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition" # change the path here to your game files folder
$gameExe = Join-Path $gameDir "DisneyInfinity3.exe"
$dllSource = "Release\bink2w32.dll"
$logFile = Join-Path $gameDir "loader.log"

Write-Host "=== 1. Closing the game (if running) ===" -ForegroundColor Cyan
Stop-Process -Name "DisneyInfinity3" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1 # give the process time to exit

Write-Host "`n=== 2. Building the DLL ===" -ForegroundColor Cyan
.\build.ps1
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed. Aborting test."
    exit $LASTEXITCODE
}

Write-Host "`n=== 3. Deploying files ===" -ForegroundColor Cyan
if (!(Test-Path $gameDir)) {
    Write-Error "Game folder not found: $gameDir"
    exit 1
}

# Back up the original bink2w32.dll to bink2w32_orig.dll (once), which our proxy forwards to.
$origDllPath = Join-Path $gameDir "bink2w32_orig.dll"
$targetDllPath = Join-Path $gameDir "bink2w32.dll"

if (!(Test-Path $origDllPath)) {
    if (Test-Path $targetDllPath) {
        Write-Host "Backing up the original bink2w32.dll to bink2w32_orig.dll..."
        Rename-Item -Path $targetDllPath -NewName "bink2w32_orig.dll"
    }
}

Write-Host "Copying the new DLL..."
Copy-Item -Path $dllSource -Destination $targetDllPath -Force

# Two Lua folders, deployed side by side and kept strictly apart:
#   api/  = the loader's own runtime (src/api/), loaded before anything else
#   mods/ = user mods, run once the game's Lua state is up
# Both targets are purged first so no stale or deleted file lingers.
foreach ($folder in @(@{ Source = "src\api"; Name = "api" }, @{ Source = "mods"; Name = "mods" })) {
    if (!(Test-Path $folder.Source)) { continue }

    Write-Host "Copying $($folder.Name)/ ..."
    $target = Join-Path $gameDir $folder.Name
    if (Test-Path $target) { Remove-Item -Path $target -Recurse -Force }
    Copy-Item -Path $folder.Source -Destination $target -Recurse -Force
}
if (Test-Path $logFile) {
    Clear-Content $logFile -ErrorAction SilentlyContinue
}

Write-Host "`n=== 4. Launching the game ===" -ForegroundColor Cyan
Write-Host "Running: $gameExe" -ForegroundColor Yellow
Start-Process -FilePath $gameExe -WorkingDirectory $gameDir

Write-Host "`n=== 5. Log monitoring (press Ctrl+C to quit) ===" -ForegroundColor Cyan
Start-Sleep -Seconds 2

if (Test-Path $logFile) {
    Get-Content $logFile -Wait -Tail 10
} else {
    Write-Host "[-] Log file not found: $logFile" -ForegroundColor Red
}
