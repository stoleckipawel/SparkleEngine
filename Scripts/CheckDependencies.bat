@echo off
:: ============================================================================
:: CheckDependencies.bat - Build toolchain dependency validator
:: ============================================================================
:: Verifies required and optional build tools are available in PATH,
:: and checks whether third-party FetchContent dependencies are synced.
::
:: Required: CMake, MSBuild, git
:: Optional: Clang (for ClangCL), clang-format, clang-tidy, git-lfs
:: Third-party status: imgui, cgltf, stb, compressonator, ktx
::
:: Usage: CheckDependencies.bat [CONTINUE]
::   CONTINUE - Suppress pause (used by parent scripts)
::
:: Environment:
::   PARENT_BATCH  - When set, suppresses pause on completion
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
::
:: Exit Codes:
::   0 - All required dependencies found
::   1 - One or more required dependencies missing
:: ============================================================================

setlocal enabledelayedexpansion

:: ---------------------------------------------------------------------------
:: Logging bootstrap
:: ---------------------------------------------------------------------------
if not defined LOG_CAPTURED (
    call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
    exit /B %ERRORLEVEL%
)

:: ---------------------------------------------------------------------------
:: Load shared configuration
:: ---------------------------------------------------------------------------
call "%~dp0Internal\Config.bat"

:: Track overall status (0 = success)
set "RC=0"

echo.
echo ============================================================
echo   Validating Build Dependencies
echo ============================================================
echo.

:: ---------------------------------------------------------------------------
:: Required: CMake
:: ---------------------------------------------------------------------------
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Install CMake and add to PATH.
    set "RC=1"
) else (
    echo [OK] CMake
)

:: ---------------------------------------------------------------------------
:: Required: MSBuild
:: ---------------------------------------------------------------------------
where msbuild >nul 2>&1
if errorlevel 1 (
    echo [ERROR] MSBuild not found. Install Visual Studio C++ workload.
    set "RC=1"
) else (
    echo [OK] MSBuild
)

:: ---------------------------------------------------------------------------
:: Optional: Clang (enables ClangCL toolset)
:: ---------------------------------------------------------------------------
where clang >nul 2>&1
if errorlevel 1 (
    echo [WARN] Clang not found. Will use MSVC toolset.
) else (
    echo [OK] Clang ^(ClangCL toolset available^)
)

:: ---------------------------------------------------------------------------
:: Optional: clang-format
:: ---------------------------------------------------------------------------
where clang-format >nul 2>&1
if errorlevel 1 (
    echo [WARN] clang-format not found. Code formatting unavailable.
) else (
    echo [OK] clang-format
)

:: ---------------------------------------------------------------------------
:: Optional: clang-tidy
:: ---------------------------------------------------------------------------
where clang-tidy >nul 2>&1
if errorlevel 1 (
    echo [WARN] clang-tidy not found. Static analysis unavailable.
) else (
    echo [OK] clang-tidy
)

:: ---------------------------------------------------------------------------
:: Required: git (for FetchContent cloning)
:: ---------------------------------------------------------------------------
where git >nul 2>&1
if errorlevel 1 (
    echo [ERROR] git not found. Required for third-party dependency fetching.
    set "RC=1"
) else (
    echo [OK] git
)

:: ---------------------------------------------------------------------------
:: Required: git-lfs (for Compressonator and KTX repos)
:: ---------------------------------------------------------------------------
where git-lfs >nul 2>&1
if errorlevel 1 (
    echo [WARN] git-lfs not found. Some third-party repos may fail to clone.
    echo         Install Git LFS: https://git-lfs.github.com/
) else (
    echo [OK] git-lfs
)

:: ---------------------------------------------------------------------------
:: Summary: Build Toolchain
:: ---------------------------------------------------------------------------
echo.
echo ============================================================
if %RC%==0 (
    echo   [SUCCESS] All required build tools found.
) else (
    echo   [ERROR] Missing required build tools. Build will fail.
)
echo ============================================================

:: ---------------------------------------------------------------------------
:: Third-party dependencies status
:: ---------------------------------------------------------------------------
:: Quick check if FetchContent deps have been downloaded.
:: This is informational — GenerateProjectFiles.bat fetches deps automatically
:: via CMake configure. Use CheckThirdParty.bat for interactive sync.
echo.
echo [LOG] Checking third-party dependency status...
set "TP_MISSING=0"
set "TP_PRESENT=0"

for %%D in (imgui cgltf stb compressonator ktx) do (
    if exist "!DEPS_DIR!\%%D-src\*" (
        :: Verify it's a valid git repo (catches interrupted clones)
        git -C "!DEPS_DIR!\%%D-src" rev-parse --is-inside-work-tree >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] %%D — corrupt or incomplete clone
            set /A "TP_MISSING+=1"
        ) else (
            echo [OK]   %%D
            set /A "TP_PRESENT+=1"
        )
    ) else (
        echo [WARN] %%D not found
        set /A "TP_MISSING+=1"
    )
)

echo.
if !TP_MISSING! GTR 0 (
    echo [WARN] !TP_MISSING! third-party dependencies missing or corrupt.
    echo        They will be fetched automatically during CMake configure.
    echo        To fix corrupt deps: Scripts\Clean.bat DEPS then re-run Setup.
    echo        To sync now: Scripts\CheckThirdParty.bat
) else (
    echo [OK]   All third-party dependencies present.
)

:: Preserve LOGFILE across endlocal
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "DEP_RC=%_TMP_RC%"

if defined PARENT_BATCH (
    exit /B %DEP_RC%
)

echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %DEP_RC%
