@echo off
REM Glisse un modele (.glb/.gltf/.obj/.zip) sur ce fichier : rig + captures dans build\captures
cd /d "%~dp0"
if "%~1"=="" (echo Glisse un modele sur ce fichier & pause & exit /b)
".venv\Scripts\python.exe" tools\capture.py "%~1" %2 %3 %4 %5
pause
