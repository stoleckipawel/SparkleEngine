@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0CodeStyle.ps1" %*
exit /b %errorlevel%
