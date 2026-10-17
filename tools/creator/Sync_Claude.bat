@echo off
REM Prepare les ressources du jeu (src\assets, non versionnees) en une seule archive pour Claude.
REM A relancer apres une regeneration des bases. Resultat : build\claude_sync\assets.zip
cd /d "%~dp0"
".venv\Scripts\python.exe" tools\pack_assets.py
pause
