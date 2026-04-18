@echo off
:: ============================================================================
:: SetupWorkspace.bat - First-time environment setup
:: ============================================================================
:: One-shot script for getting a working build from a fresh clone.
:: Validates the host toolchain and refreshes the build files.
::
:: Idempotent - safe to run multiple times. Skips work already done.
::
:: Usage: SetupWorkspace.bat
::
:: Flow:
::   1. Validate required build tools (CMake, MSBuild, git)
::   2. Run GenerateSolution.bat to generate or refresh the build files
::   3. Display next steps
::
:: Environment:
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
::
:: Exit Codes:
::   0 - Setup completed successfully
::   1 - Setup failed
:: ============================================================================

setlocal enabledelayedexpansion

:: ---------------------------------------------------------------------------
:: Logging bootstrap
:: ---------------------------------------------------------------------------
if not defined LOG_CAPTURED (
    call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
    set "BOOTSTRAP_RC=!ERRORLEVEL!"
    exit /B !BOOTSTRAP_RC!
)

:: ---------------------------------------------------------------------------
:: Load shared configuration
:: ---------------------------------------------------------------------------
call "%~dp0Internal\Config.bat"

echo.
echo ============================================================
echo   Sparkle Engine - Workspace Setup
echo ============================================================
echo.

:: ---------------------------------------------------------------------------
:: Step 1: Validate build toolchain
:: ---------------------------------------------------------------------------
echo [LOG] Step 1/2: Validating build tools...
echo.
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\Internal\CheckToolchain.bat" CONTINUE
set "DEP_RC=!ERRORLEVEL!"
set "PARENT_BATCH="

if "!DEP_RC!" NEQ "0" (
    echo.
    echo [ERROR] Required build tools are missing.
    echo         Install the tools marked [ERROR] above, then re-run SetupWorkspace.bat.
    echo         Inspect the validation output above for the missing tool details.
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Step 2: Configure build files
:: ---------------------------------------------------------------------------
echo.
echo [LOG] Step 2/2: Refreshing build files and syncing dependencies...
echo.
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\GenerateSolution.bat" CONTINUE
set "GEN_RC=!ERRORLEVEL!"
set "PARENT_BATCH="

if "!GEN_RC!" NEQ "0" (
    echo.
    echo [ERROR] GenerateSolution step failed.
    echo         Check the CMake output above for the root cause.
    echo         Possible fixes:
    echo           - Clean stale cache: Scripts\CleanWorkspace.bat BUILD
    echo           - Full reset:        Scripts\CleanWorkspace.bat ALL
    echo           - Re-run bootstrap:  Scripts\SetupWorkspace.bat
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Setup complete
:: ---------------------------------------------------------------------------
echo.
echo ============================================================
echo   [SUCCESS] Workspace Setup Complete
echo ============================================================
echo.
echo   Useful scripts:
echo     GenerateSolution.bat      - Generate or refresh build files
echo     BuildProject.bat          - Build one project's editor/runtime targets
echo     CookAllAssets.bat         - Run the full shader, texture, and scene cook flow
echo     Cook\CookShaders.bat      - Cook shader packages for a project
echo     Cook\CookTextures.bat     - Cook texture assets for a project
echo     Cook\CookAssets.bat       - Cook scene, mesh, and material assets for a project
echo     RunClangFormat.bat        - Run clang-format
echo     CleanWorkspace.bat        - Clean build artifacts
echo.
echo ============================================================

:: ---------------------------------------------------------------------------
:: Prompt to open VS solution (SetupWorkspace.bat is always interactive)
:: ---------------------------------------------------------------------------
echo.
echo ============================================================
echo   Open Visual Studio?
echo ============================================================
echo.
echo   Y^) Yes - Open !PROJECT_NAME!.sln in Visual Studio
echo   N^) No  - Exit
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

:AFTER_VS_PROMPT
set "EXIT_RC=0"
goto :FINISH

:: ============================================================================
:: Clean exit with proper endlocal handling
:: ============================================================================
:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%"

echo.
if "%EXIT_RC%"=="0" (
    echo [LOG] SetupWorkspace completed successfully.
) else (
    echo [ERROR] SetupWorkspace failed. See output above for details.
)
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%