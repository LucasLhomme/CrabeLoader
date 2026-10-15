@echo off
cd /d "%~dp0"
".venv\Scripts\python.exe" src\main.py
if errorlevel 1 (
  echo.
  echo L appli s est arretee. Details dans build\creator.log
  pause
)
