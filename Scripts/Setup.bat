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
    echo         Install the missing tools listed above and re-run Setup.bat.
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
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Step 3: Generate Visual Studio solution
:: ---------------------------------------------------------------------------
echo.
echo [LOG] Step 3/3: Generating Visual Studio solution...
echo.

:: Extract project name for solution file check
set "PROJECT_NAME="
for /f "tokens=2 delims=( " %%P in ('findstr /i "project(" "!ROOT_DIR!\CMakeLists.txt"') do (
    set "RAW_NAME=%%P"
)
for /f "delims=) tokens=1" %%A in ("!RAW_NAME!") do set "PROJECT_NAME=%%A"
set "PROJECT_NAME=!PROJECT_NAME: =!"
set "SOLUTION_FILE=!BUILD_DIR!\!PROJECT_NAME!.sln"

:: Only generate if solution doesn't exist or CMakeCache is missing
if exist "!SOLUTION_FILE!" (
    if exist "!BUILD_DIR!\CMakeCache.txt" (
        echo [OK]   Solution already exists: !SOLUTION_FILE!
        echo [LOG] Skipping generation — solution is up to date.
        goto :SETUP_COMPLETE
    )
)

:: Run cmake configure
if not exist "!BUILD_DIR!" mkdir "!BUILD_DIR!"

pushd "!BUILD_DIR!"
if "!USE_CLANG!"=="1" (
    echo [LOG] CMake: -G "!GENERATOR!" -A !ARCH! -T ClangCL
    cmake -G "!GENERATOR!" -A !ARCH! -T ClangCL "!ROOT_DIR!"
) else (
    echo [LOG] CMake: -G "!GENERATOR!" -A !ARCH!
    cmake -G "!GENERATOR!" -A !ARCH! "!ROOT_DIR!"
)
set "CMAKE_RC=!ERRORLEVEL!"
popd

if "!CMAKE_RC!" NEQ "0" (
    echo.
    echo [ERROR] CMake generation failed.
    set "EXIT_RC=1"
    goto :FINISH
)

echo [LOG] Solution generated: !SOLUTION_FILE!

:: ---------------------------------------------------------------------------
:: Setup complete
:: ---------------------------------------------------------------------------
:SETUP_COMPLETE
echo.
echo ============================================================
echo   [SUCCESS] Setup Complete
echo ============================================================
echo.
echo   Next steps:
echo     1. Open build\!PROJECT_NAME!.sln in Visual Studio
echo     2. Set DemoProject as startup project
echo     3. Build and run ^(F5^)
echo.
echo   Useful scripts:
echo     GenerateProjectFiles.bat  - Regenerate VS solution
echo     BuildProjects.bat         - Build from command line
echo     CreateNewProject.bat      - Create a new project
echo     Clean.bat                 - Clean build artifacts
echo.
echo ============================================================

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

