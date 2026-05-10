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

call "%~dp0..\Core\Config.bat"

set "DEVENV_EXE="

if defined VS_INSTALLATION_PATH if exist "!VS_INSTALLATION_PATH!\Common7\IDE\devenv.exe" (
    set "DEVENV_EXE=!VS_INSTALLATION_PATH!\Common7\IDE\devenv.exe"
)

if exist "!VSWHERE_EXE!" (
    for /f "usebackq delims=" %%I in (`""!VSWHERE_EXE!" -latest -products * -version "!VS_VERSION_RANGE!" -requires "!VS_CPP_COMPONENT!" -find Common7\IDE\devenv.exe"`) do (
        if not defined DEVENV_EXE set "DEVENV_EXE=%%~fI"
    )
)

if defined DEVENV_EXE (
    start "" "!DEVENV_EXE!" "!SOLUTION_FILE!"
    set "OPEN_RC=!ERRORLEVEL!"
) else (
    echo [ERROR] Visual Studio was not found. Install Visual Studio 2022 or newer with the C++ workload.
    set "OPEN_RC=1"
)

endlocal & exit /B %OPEN_RC%
