@echo off
:: ============================================================================
:: Setup.bat - First-time environment setup
:: ============================================================================
:: One-shot script for getting a working build from a fresh clone.
:: Validates tools, fetches third-party dependencies, and generates
:: the Visual Studio solution.
::
:: Idempotent — safe to run multiple times. Skips work already done.
::
:: Usage: Setup.bat
::
:: Flow:
::   1. Validate required build tools (CMake, MSBuild, git)
::   2. Fetch third-party dependencies via CMake FetchContent
::   3. Generate Visual Studio solution
::   4. Display next steps
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
    exit /B %ERRORLEVEL%
)

:: ---------------------------------------------------------------------------
:: Load shared configuration
:: ---------------------------------------------------------------------------
call "%~dp0Internal\Config.bat"

echo.
echo ============================================================
echo   Sparkle Engine — First-Time Setup
echo ============================================================
echo.

:: ---------------------------------------------------------------------------
:: Step 1: Validate build dependencies
:: ---------------------------------------------------------------------------
echo [LOG] Step 1/3: Validating build tools...
echo.
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\CheckDependencies.bat" CONTINUE
set "DEP_RC=!ERRORLEVEL!"
set "PARENT_BATCH="

if "!DEP_RC!" NEQ "0" (
    echo.
    echo [ERROR] Required build tools are missing.
    echo         Install the tools marked [ERROR] above, then re-run Setup.bat.
    echo         For details: Scripts\CheckDependencies.bat
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Step 2: Fetch third-party dependencies
:: ---------------------------------------------------------------------------
echo.
echo [LOG] Step 2/3: Fetching third-party dependencies...
echo.
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\CheckThirdParty.bat"
set "TP_RC=!ERRORLEVEL!"
set "PARENT_BATCH="

if "!TP_RC!" NEQ "0" (
    echo.
    echo [ERROR] Third-party dependency fetch failed.
    echo         Check the output above for errors.
    echo         Possible fixes:
    echo           - Verify internet connection
    echo           - Clean corrupt deps: Scripts\Clean.bat DEPS
    echo           - Manual sync:        Scripts\CheckThirdParty.bat
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Step 3: Generate Visual Studio solution
:: ---------------------------------------------------------------------------
echo.
echo [LOG] Step 3/3: Generating Visual Studio solution...
echo.

:: Delegate to GenerateProjectFiles.bat — single source of truth for cmake
:: invocation, toolset detection, and solution generation.
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\GenerateProjectFiles.bat" CONTINUE
set "GEN_RC=!ERRORLEVEL!"
set "PARENT_BATCH="

if "!GEN_RC!" NEQ "0" (
    echo.
    echo [ERROR] Solution generation failed.
    echo         Check the CMake output above for the root cause.
    echo         Possible fixes:
    echo           - Clean stale cache: Scripts\Clean.bat BUILD
    echo           - Full reset:        Scripts\Clean.bat ALL
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Setup complete
:: ---------------------------------------------------------------------------
echo.
echo ============================================================
echo   [SUCCESS] Setup Complete
echo ============================================================
echo.
echo   Useful scripts:
echo     GenerateProjectFiles.bat  - Regenerate VS solution
echo     BuildProjects.bat         - Build from command line
echo     CreateNewProject.bat      - Create a new project
echo     Clean.bat                 - Clean build artifacts
echo.
echo ============================================================

:: ---------------------------------------------------------------------------
:: Prompt to open VS solution (Setup.bat is always interactive)
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
    echo [LOG] Setup completed successfully.
) else (
    echo [ERROR] Setup failed. See output above for details.
)
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%

