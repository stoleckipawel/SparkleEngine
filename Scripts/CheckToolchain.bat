@echo off
:: ============================================================================
:: CheckToolchain.bat - Build toolchain validator
:: ============================================================================
:: Verifies required and optional host build tools are available in PATH.
:: Third-party dependency sync is intentionally handled separately by
:: SyncThirdParty.bat.
::
:: Required: CMake, MSBuild, git
:: Optional: Clang (for ClangCL), clang-format, clang-tidy, git-lfs
::
:: Usage: CheckToolchain.bat [CONTINUE]
::   CONTINUE - Suppress pause (used by parent scripts)
:: ============================================================================

setlocal enabledelayedexpansion

if /I "%~1"=="CONTINUE" if not defined PARENT_BATCH set "PARENT_BATCH=1"

if not defined LOG_CAPTURED (
    call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
    exit /B %ERRORLEVEL%
)

call "%~dp0Internal\Config.bat"

set "RC=0"

echo.
echo ============================================================
echo   Validating Build Toolchain
echo ============================================================
echo.

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Install CMake and add to PATH.
    echo         Download: https://cmake.org/download/
    set "RC=1"
) else (
    for /f "tokens=3" %%V in ('cmake --version 2^>nul ^| findstr /i /c:"cmake version"') do (
        echo [OK] CMake %%V
    )
)

where msbuild >nul 2>&1
if errorlevel 1 (
    echo [ERROR] MSBuild not found. Install Visual Studio with C++ workload.
    echo         Run Visual Studio Installer ^> Modify ^> Desktop development with C++.
    set "RC=1"
) else (
    echo [OK] MSBuild
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "!VSWHERE!" (
    "!VSWHERE!" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Visual Studio C++ tools not found.
        echo         Run Visual Studio Installer ^> Modify ^> Desktop development with C++.
        set "RC=1"
    ) else (
        echo [OK] Visual Studio C++ tools
    )
) else (
    echo [WARN] vswhere not found - cannot verify Visual Studio installation.
    echo        Ensure Visual Studio 2022+ with C++ workload is installed.
)

where clang >nul 2>&1
if errorlevel 1 (
    echo [WARN] Clang not found. Configure will fall back to MSVC.
) else (
    echo [OK] Clang ^(ClangCL toolset available^)
)

where clang-format >nul 2>&1
if errorlevel 1 (
    echo [WARN] clang-format not found. Formatting command will be unavailable.
) else (
    echo [OK] clang-format
)

where clang-tidy >nul 2>&1
if errorlevel 1 (
    echo [WARN] clang-tidy not found. Static analysis command will be unavailable.
) else (
    echo [OK] clang-tidy
)

where git >nul 2>&1
if errorlevel 1 (
    echo [ERROR] git not found. Required for FetchContent cloning.
    echo         Download: https://git-scm.com/download/win
    set "RC=1"
) else (
    for /f "tokens=3" %%V in ('git --version 2^>nul') do (
        echo [OK] git %%V
    )
)

where git-lfs >nul 2>&1
if errorlevel 1 (
    echo [WARN] git-lfs not found. Not required ^(LFS downloads are skipped during fetch^).
) else (
    echo [OK] git-lfs
)

echo.
echo ============================================================
if %RC%==0 (
    echo   [SUCCESS] All required build tools found.
) else (
    echo   [ERROR] Missing required build tools. Configure and build steps will fail.
)
echo ============================================================
echo.
echo [LOG] Third-party dependency sync is handled separately by SyncThirdParty.bat.

set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "TOOLCHAIN_RC=%_TMP_RC%"

if defined PARENT_BATCH (
    exit /B %TOOLCHAIN_RC%
)

echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %TOOLCHAIN_RC%