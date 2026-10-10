@echo off
setlocal
if not "%~1"=="" goto explicit
set "styleScope="

echo SparkleEngine code style
echo.
echo 1. Check entire owned codebase
echo 2. Format entire owned codebase
echo 3. Check staged check-in files
echo 4. Format staged check-in files
echo 0. Exit
echo.
choice /c 12340 /n /m "Choose action and scope: "
if errorlevel 255 exit /b 1
if errorlevel 5 exit /b 0
if errorlevel 4 goto formatStaged
if errorlevel 3 goto checkStaged
if errorlevel 2 goto formatAll
if errorlevel 1 goto checkAll
exit /b 1

:formatStaged
set "styleMode=Format"
set "styleScope=-Staged"
goto interactive

:checkStaged
set "styleMode=Check"
set "styleScope=-Staged"
goto interactive

:formatAll
set "styleMode=Format"
goto interactive

:checkAll
set "styleMode=Check"

:interactive
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0CodeStyle.ps1" -Mode %styleMode% %styleScope%
set "styleResult=%errorlevel%"
echo.
pause
exit /b %styleResult%

:explicit
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0CodeStyle.ps1" %*
exit /b %errorlevel%
