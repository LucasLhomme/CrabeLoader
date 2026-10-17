@echo off
REM Disney Infinity Creator launcher. First run: creates .venv and installs the dependencies (run.ps1).
cd /d "%~dp0"
if not exist ".venv\Scripts\python.exe" (
  echo First launch: installing Python dependencies...
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1" -SetupOnly
  if errorlevel 1 (
    echo.
    echo Setup failed. Python 3.12+ must be installed and on PATH.
    pause
    exit /b 1
  )
)
".venv\Scripts\python.exe" src\main.py
if errorlevel 1 (
  echo.
  echo Creator stopped unexpectedly. Details in build\creator.log
  pause
)
