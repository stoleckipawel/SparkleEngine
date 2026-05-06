@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    echo [ERROR] OpenVisualStudio.bat requires a solution path.
    endlocal
    exit /B 1
)

set "SOLUTION_FILE=%~f1"
if not exist "!SOLUTION_FILE!" (
    echo [ERROR] Solution file not found: !SOLUTION_FILE!
    endlocal
    exit /B 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "DEVENV_EXE="

if exist "!VSWHERE!" (
    for /f "usebackq delims=" %%I in (`"!VSWHERE!" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\IDE\devenv.exe`) do (
        if not defined DEVENV_EXE set "DEVENV_EXE=%%~fI"
    )
)

if defined DEVENV_EXE (
    start "" "!DEVENV_EXE!" "!SOLUTION_FILE!"
    set "OPEN_RC=!ERRORLEVEL!"
) else (
    start "" "!SOLUTION_FILE!"
    set "OPEN_RC=!ERRORLEVEL!"
)

endlocal & exit /B %OPEN_RC%