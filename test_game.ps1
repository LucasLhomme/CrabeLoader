$ErrorActionPreference = "Stop"

$gameDir = "D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition"
$gameExe = Join-Path $gameDir "DisneyInfinity3.exe"
$dllCandidates = @(
    "build\Release\bink2w32.dll",
    "Release\bink2w32.dll",
    "build\vs2022-dll\Release\bink2w32.dll"
)

Write-Host "=== 1. Fermeture du jeu (si en cours) ===" -ForegroundColor Cyan
Stop-Process -Name "DisneyInfinity3" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 800

Write-Host "=== 2. Build de la DLL (Release Win32) ===" -ForegroundColor Cyan
cmake --build build --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Error "Échec de la compilation."
    exit $LASTEXITCODE
}

Write-Host "=== 3. Déploiement ===" -ForegroundColor Cyan
$origDll = Join-Path $gameDir "bink2w32_orig.dll"
$targetDll = Join-Path $gameDir "bink2w32.dll"
if (!(Test-Path $origDll) -and (Test-Path $targetDll)) {
    Copy-Item -Path $targetDll -Destination $origDll -Force
}

$dllSrc = $dllCandidates | Where-Object { Test-Path $_ } | Sort-Object { (Get-Item $_).LastWriteTime } -Descending | Select-Object -First 1
Copy-Item -Path $dllSrc -Destination $targetDll -Force
Write-Host "[OK] bink2w32.dll -> $gameDir" -ForegroundColor Green

# Nettoyage de l'ancien dossier api (l'API Lua est désormais compilée dans la DLL)
$apiDir = Join-Path $gameDir "api"
if (Test-Path $apiDir) {
    Remove-Item -Path $apiDir -Recurse -Force -ErrorAction SilentlyContinue
}

# Déploiement dans mods/ uniquement
$modsDir = Join-Path $gameDir "mods"
if (!(Test-Path $modsDir)) { New-Item -ItemType Directory -Force -Path $modsDir | Out-Null }
if (Test-Path "mods") {
    Copy-Item -Path "mods\*.lua" -Destination $modsDir -Force -ErrorAction SilentlyContinue
    # Mod folders, not just loose scripts: crabe_heroes ships characters/ and no
    # Lua of its own, and the loader reads them from mods/<name>/characters/.
    # Contents into an explicit destination, not the folder into $modsDir:
    # Copy-Item -Recurse nests (mods/x/x) when the destination already exists.
    Get-ChildItem -Path "mods" -Directory |
        Where-Object { $_.Name -notmatch '^[._]' } |
        ForEach-Object {
            $modDest = Join-Path $modsDir $_.Name
            if (!(Test-Path $modDest)) { New-Item -ItemType Directory -Force -Path $modDest | Out-Null }
            Copy-Item -Path (Join-Path $_.FullName '*') -Destination $modDest -Recurse -Force
        }
}
$menuMod = "..\CrabeMenu\mods\crabemenu.lua"
if (Test-Path $menuMod) {
    Copy-Item -Path $menuMod -Destination (Join-Path $modsDir "crabemenu.lua") -Force
}

# Synchro skilltrees si présent. characters/ n'est plus déployé à la racine du
# jeu : un personnage appartient au mod qui le fournit (mods/<nom>/characters/).
foreach ($folder in @("skilltrees")) {
    if (Test-Path $folder) {
        $dest = Join-Path $gameDir $folder
        if (!(Test-Path $dest)) { New-Item -ItemType Directory -Force -Path $dest | Out-Null }
        Copy-Item -Path "$folder\*" -Destination $dest -Recurse -Force
    }
}

Write-Host "=== 4. Lancement du jeu ===" -ForegroundColor Green
if (Get-Process -Name "steam" -ErrorAction SilentlyContinue) {
    Start-Process "steam://rungameid/541670"
} else {
    Start-Process -FilePath $gameExe -WorkingDirectory $gameDir
}
Write-Host "Jeu lance avec succes !" -ForegroundColor Green
