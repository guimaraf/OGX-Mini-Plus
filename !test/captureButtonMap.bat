@echo off
setlocal

set "SCRIPT=%~dp0captureButtonMap.ps1"
powershell -ExecutionPolicy Bypass -File "%SCRIPT%"

pause
