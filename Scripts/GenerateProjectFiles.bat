@echo off
:: ============================================================================
:: GenerateProjectFiles.bat - Visual Studio solution generator
:: ============================================================================
:: Generates the VS solution via CMake configure. Incremental by default —
:: preserves existing build artifacts and third-party dependencies.
::
::   1. Validates required build dependencies
::   2. Configures toolset (ClangCL preferred, MSVC fallback)
::   3. Generates Visual Studio solution via CMake
::
:: Usage: GenerateProjectFiles.bat [CONTINUE]
::   CONTINUE - Suppress interactive prompts (used by parent scripts)
::
:: Environment:
::   PARENT_BATCH  - When set, suppresses pause on completion
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
::
:: Exit Codes:
::   0 - Solution generated successfully
::   1 - Generation failed
:: ============================================================================

setlocal enabledelayedexpansion

:: ---------------------------------------------------------------------------
:: Determine interactive vs non-interactive mode
:: ---------------------------------------------------------------------------
:: INTERACTIVE=0 suppresses prompts (dep check, Open VS, pause) when called
:: from a parent script. Callers signal this via PARENT_BATCH or CONTINUE arg.
set "INTERACTIVE=1"
if defined PARENT_BATCH set "INTERACTIVE=0"
if /I "%~1"=="CONTINUE" set "INTERACTIVE=0"

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

:: ---------------------------------------------------------------------------
:: Step 1: Validate build dependencies (standalone mode only)
:: ---------------------------------------------------------------------------
:: When called from a parent (Setup.bat, BuildProjectsImpl.bat), the caller
:: has already validated tools. Skip the redundant check to keep logs clean.
if "!INTERACTIVE!"=="1" (
    echo [LOG] Checking build dependencies...
    set "PARENT_BATCH=1"
    call "%~dp0CheckDependencies.bat" CONTINUE
    if errorlevel 1 (
        set "PARENT_BATCH="
        echo [ERROR] Dependency check failed. Install the missing tools above.
        set "EXIT_RC=1"
        goto :FINISH
    )
    set "PARENT_BATCH="
)

:: ---------------------------------------------------------------------------
:: Step 2: Log build environment
:: ---------------------------------------------------------------------------
if "!USE_CLANG!"=="1" (
    echo [LOG] Toolset: ClangCL
) else (
    echo [LOG] Toolset: MSVC ^(Clang not found^)
)

:: PROJECT_NAME and SOLUTION_FILE are set by Config.bat
echo [LOG] Project: !PROJECT_NAME!

:: ---------------------------------------------------------------------------
:: Step 3: Generate solution via CMake configure
:: ---------------------------------------------------------------------------
:: Always run cmake — incremental configures are fast and ensure the
:: solution stays in sync with CMakeLists.txt changes.
if not exist "!BUILD_DIR!\CMakeCache.txt" (
    echo [LOG] No CMake cache found. Running full configure...
) else if not exist "!SOLUTION_FILE!" (
    echo [LOG] Solution file missing. Regenerating...
) else (
    echo [LOG] Running incremental configure...
)

if not exist "!BUILD_DIR!" (
    echo [LOG] Creating build directory: !BUILD_DIR!
    mkdir "!BUILD_DIR!"
    if errorlevel 1 (
        echo [ERROR] Failed to create build directory.
        set "EXIT_RC=1"
        goto :FINISH
    )
)

pushd "!BUILD_DIR!"

if "!USE_CLANG!"=="1" (
    echo [LOG] CMake: -G "!GENERATOR!" -A !ARCH! -T ClangCL -Wno-dev
    cmake -G "!GENERATOR!" -A !ARCH! -T ClangCL -Wno-dev "!ROOT_DIR!"
) else (
    echo [LOG] CMake: -G "!GENERATOR!" -A !ARCH! -Wno-dev
    cmake -G "!GENERATOR!" -A !ARCH! -Wno-dev "!ROOT_DIR!"
)

set "CMAKE_RC=!ERRORLEVEL!"
popd

if "!CMAKE_RC!" NEQ "0" (
    echo.
    echo [ERROR] CMake generation failed ^(exit code !CMAKE_RC!^).
    echo         Check the CMake output above for the root cause.
    echo         Common fixes:
    echo           - Install missing tools: Scripts\CheckDependencies.bat
    echo           - Clean stale cache:     Scripts\Clean.bat BUILD
    echo           - Full reset:            Scripts\Clean.bat ALL
    set "EXIT_RC=1"
    goto :FINISH
)

echo [LOG] Solution generated: !SOLUTION_FILE!

:: ---------------------------------------------------------------------------
:: Optional: Open solution after generation (interactive mode only)
:: ---------------------------------------------------------------------------
if "!INTERACTIVE!"=="1" (
    echo.
    echo ============================================================
    echo   Open Visual Studio?
    echo ============================================================
    echo.
    echo   Y^) Yes - Open the generated solution
    echo   N^) No  - Continue without opening
    echo.
    echo ============================================================

    :OPEN_VS_PROMPT
    set "OPEN_VS="
    set /P "OPEN_VS=Enter choice [Y/N]: "

    if /I "!OPEN_VS!"=="Y" (
        echo.
        echo [LOG] Opening: !SOLUTION_FILE!
        start "" "!SOLUTION_FILE!"
        goto :AFTER_VS_PROMPT
    )
    if /I "!OPEN_VS!"=="N" goto :AFTER_VS_PROMPT
    if "!OPEN_VS!"=="" goto :AFTER_VS_PROMPT

    echo [WARN] Invalid input. Please enter Y or N.
    goto :OPEN_VS_PROMPT
)
:AFTER_VS_PROMPT

echo.
echo [LOG] GenerateProjectFiles.bat completed.
set "EXIT_RC=0"
goto :FINISH

:: ============================================================================
:: Clean exit with proper endlocal handling
:: ============================================================================
:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
set "_TMP_INTERACTIVE=%INTERACTIVE%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%" & set "_INTERACTIVE=%_TMP_INTERACTIVE%"

:: In non-interactive mode, return silently to caller
if "%_INTERACTIVE%"=="0" (
    set "_INTERACTIVE="
    exit /B %EXIT_RC%
)
set "_INTERACTIVE="

echo.
if "%EXIT_RC%"=="0" (
    echo ============================================================
    echo   [SUCCESS] Solution generation completed.
    echo ============================================================
) else (
    echo ============================================================
    echo   [ERROR] Solution generation failed.
    echo ============================================================
)
echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%

