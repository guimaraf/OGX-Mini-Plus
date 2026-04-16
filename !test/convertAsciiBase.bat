@echo off
setlocal

REM Caminho do arquivo .ps1 (na mesma pasta do .bat)
set "SCRIPT=%~dp0convertAscii.ps1"

REM Executa o PowerShell permitindo rodar o script
powershell -ExecutionPolicy Bypass -File "%SCRIPT%"

pause