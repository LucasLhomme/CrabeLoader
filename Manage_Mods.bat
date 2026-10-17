@echo off
REM Outil a part (hors Creator) : liste, desactive, supprime (corbeille) et nettoie les personnages Creator.
REM Apres chaque changement, les archives partagees (mods\zz_creator_shared : textures et icones) sont refaites,
REM donc un personnage supprime ne laisse rien dans le jeu. Ferme le jeu avant.
chcp 65001 >nul
cd /d "%~dp0"
".venv\Scripts\python.exe" tools\manage_mods.py %*
pause
