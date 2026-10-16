@echo off
REM Outil a part (hors Creator) : glisse un modele (.glb/.gltf/.obj/.dae/.zip) sur ce fichier.
REM Il construit le rig, joue toutes les animations dessus et ecrit un rapport dans build\audit\<modele>\
REM (report.txt + images des pires images). Envoie ce dossier pour diagnostiquer un modele.
REM Base differente : Audit_Animations.bat modele.obj GOG_Gamora
cd /d "%~dp0"
if "%~1"=="" (echo Glisse un modele sur ce fichier & pause & exit /b)
set BASE=%~2
if "%BASE%"=="" set BASE=TCW_Anakin
".venv\Scripts\python.exe" tools\audit.py "%~1" --base %BASE%
echo.
echo Rapport dans build\audit
pause
