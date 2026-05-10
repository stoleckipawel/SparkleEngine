@echo off
:: ============================================================================
:: GenerateSolution.bat - Generate or refresh Visual Studio build files
:: ============================================================================
:: Runs the repository CMake configure step. This is the single public owner
:: of generator/toolset selection and solution generation.
:: Most contributors will not run this directly every day because
:: SetupWorkspace.bat, BuildProject.bat, and the Scripts\Cook entrypoints
:: already call it when needed.
::
:: Usage: GenerateSolution.bat [CONTINUE]
::   CONTINUE - Suppress interactive prompts (used by parent scripts)
:: ============================================================================

setlocal enabledelayedexpansion

set "INTERACTIVE=1"
if defined PARENT_BATCH set "INTERACTIVE=0"
if /I "%~1"=="CONTINUE" set "INTERACTIVE=0"

if not defined LOG_CAPTURED (
    call "%~dp0Internal\Core\BootstrapLog.bat" "%~f0" %*
    set "BOOTSTRAP_RC=!ERRORLEVEL!"
    exit /B !BOOTSTRAP_RC!
)

call "%~dp0Internal\Core\Config.bat"

if "!INTERACTIVE!"=="1" (
    echo [LOG] Checking build toolchain...
    set "PARENT_BATCH=1"
    call "%~dp0Internal\Toolchain\CheckToolchain.bat" CONTINUE
    if errorlevel 1 (
        set "PARENT_BATCH="
        echo [ERROR] Toolchain validation failed. Install the missing tools above.
        echo         Re-run SetupWorkspace.bat for the full workspace bootstrap path.
        set "EXIT_RC=1"
        goto :FINISH
    )
    set "PARENT_BATCH="
)

if "!USE_CLANG!"=="1" (
    echo [LOG] Toolset: ClangCL
) else (
    echo [LOG] Toolset: MSVC ^(Clang not found^)
)
echo [LOG] Project: !PROJECT_NAME!

if not exist "!BUILD_DIR!\CMakeCache.txt" (
    echo [LOG] No CMake cache found. Running full configure...
) else if not exist "!SOLUTION_FILE!" (
    echo [LOG] Solution file missing. Regenerating...
) else (
    echo [LOG] Running incremental configure...
)

call "%~dp0Internal\Build\CMakeHelpers.bat" Configure
set "CONFIGURE_RC=!ERRORLEVEL!"

if "!CONFIGURE_RC!" NEQ "0" (
    echo.
    echo [ERROR] CMake configure failed ^(exit code !CONFIGURE_RC!^).
    echo         Common fixes:
    echo           - Re-run workspace setup: Scripts\SetupWorkspace.bat
    echo           - Clean stale cache:      Scripts\CleanWorkspace.bat BUILD
    echo           - Full reset:             Scripts\CleanWorkspace.bat ALL
    set "EXIT_RC=1"
    goto :FINISH
)

echo [LOG] Solution generated: !SOLUTION_FILE!

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
        call "%~dp0Internal\Toolchain\OpenVisualStudio.bat" "!SOLUTION_FILE!"
        goto :AFTER_VS_PROMPT
    )
    if /I "!OPEN_VS!"=="N" goto :AFTER_VS_PROMPT
    if "!OPEN_VS!"=="" goto :AFTER_VS_PROMPT

    echo [WARN] Invalid input. Please enter Y or N.
    goto :OPEN_VS_PROMPT
)

:AFTER_VS_PROMPT
set "EXIT_RC=0"
goto :FINISH

:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
set "_TMP_INTERACTIVE=%INTERACTIVE%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%" & set "_INTERACTIVE=%_TMP_INTERACTIVE%"

if "%_INTERACTIVE%"=="0" (
    set "_INTERACTIVE="
    exit /B %EXIT_RC%
)
set "_INTERACTIVE="

echo.
if "%EXIT_RC%"=="0" (
    echo ============================================================
    echo   [SUCCESS] GenerateSolution completed successfully.
    echo ============================================================
) else (
    echo ============================================================
    echo   [ERROR] GenerateSolution failed.
    echo ============================================================
)
echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%
