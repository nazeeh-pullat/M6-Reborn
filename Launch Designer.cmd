@echo off
setlocal
cd /d "%~dp0"
if exist ".venv\Scripts\pythonw.exe" (
  start "" ".venv\Scripts\pythonw.exe" "desktop\launch.pyw"
) else (
  python "desktop\launch.pyw"
  if errorlevel 1 pause
)
