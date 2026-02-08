@echo off
:: ============================================================================
:: Clean.bat - Unified build artifact cleanup utility
:: ============================================================================
:: Removes generated build artifacts with fine-grained control:
::   1) Build artifacts only — build/ (except _deps/), bin/, .vs/
::   2) Third-party deps only — build/_deps/
::   3) Everything — full clean (build/, bin/, .vs/ including _deps/)
::   4) Pristine — nuclear option (build/, bin/, .vs/, logs/)
::
:: Usage: Clean.bat [BUILD|DEPS|ALL|PRISTINE]
::   BUILD    - Remove build artifacts only (preserve third-party deps)
::   DEPS     - Remove third-party dependencies only
::   ALL      - Remove everything (build + deps + bin)
::   PRISTINE - Remove absolutely everything (returns to freshly-cloned state)
::   (no arg) - Show interactive menu
::
:: Environment:
::   PARENT_BATCH  - When set, suppresses pause on completion
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
::
:: Exit Codes:
::   0 - Cleanup completed (or nothing to clean, or user cancelled)
::   1 - Cleanup failed (locked files)
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

:: ---------------------------------------------------------------------------
:: Parse argument or show menu
:: ---------------------------------------------------------------------------
set "CLEAN_MODE="
if /I "%~1"=="BUILD"    set "CLEAN_MODE=BUILD"
if /I "%~1"=="DEPS"     set "CLEAN_MODE=DEPS"
if /I "%~1"=="ALL"      set "CLEAN_MODE=ALL"
if /I "%~1"=="PRISTINE" set "CLEAN_MODE=PRISTINE"

if defined CLEAN_MODE goto :EXECUTE_CLEAN

:: ---------------------------------------------------------------------------
:: Interactive menu
:: ---------------------------------------------------------------------------
:CLEAN_MENU
echo.
echo ============================================================
echo   Clean Options
echo ============================================================
echo.
echo   1^) Build artifacts only   ^(build/, bin/, .vs/^)
echo      Preserves third-party deps in build\_deps\.
echo.
echo   2^) Third-party deps only  ^(build/_deps/, ~64 MB^)
echo      Re-download required on next cmake configure.
echo.
echo   3^) Everything             ^(build + deps + bin^)
echo      Full rebuild required after this.
echo.
echo   4^) Pristine               ^(returns to freshly-cloned state^)
echo      Removes build/, bin/, .vs/, logs/ — everything generated.
echo.
echo ============================================================

set "CLEAN_SEL="
set /P "CLEAN_SEL=Enter choice [1-4]: "

if "!CLEAN_SEL!"=="1" set "CLEAN_MODE=BUILD"
if "!CLEAN_SEL!"=="2" set "CLEAN_MODE=DEPS"
if "!CLEAN_SEL!"=="3" set "CLEAN_MODE=ALL"
if "!CLEAN_SEL!"=="4" set "CLEAN_MODE=PRISTINE"

if not defined CLEAN_MODE (
    echo.
    echo [WARN] Invalid selection: '!CLEAN_SEL!' - Please enter 1, 2, 3, or 4.
    goto :CLEAN_MENU
)

:: ---------------------------------------------------------------------------
:: Confirm before cleaning (unless PARENT_BATCH is set)
:: ---------------------------------------------------------------------------
if defined PARENT_BATCH goto :EXECUTE_CLEAN

echo.
if "!CLEAN_MODE!"=="BUILD" (
    echo [LOG] Selected: Build artifacts only
    echo       Will remove: build/ ^(except _deps/^), bin/, .vs/
) else if "!CLEAN_MODE!"=="DEPS" (
    echo [LOG] Selected: Third-party deps only
    echo       Will remove: build/_deps/
    echo       Re-download: ~64 MB, 1-3 min to re-sync
) else if "!CLEAN_MODE!"=="ALL" (
    echo [LOG] Selected: Everything
    echo       Will remove: build/, bin/, .vs/
    echo       Includes all third-party deps ^(~64 MB re-download^)
) else (
    echo [LOG] Selected: Pristine ^(nuclear option^)
    echo       Will remove: build/, bin/, .vs/, logs/
    echo       Returns project to freshly-cloned state.
)

echo.
echo ============================================================
echo   Proceed with cleanup?
echo ============================================================
echo.
echo   Y^) Yes - Remove selected items
echo   N^) No  - Cancel
echo.
echo ============================================================

:CONFIRM_PROMPT
set "CONFIRM="
set /P "CONFIRM=Enter choice [Y/N]: "

if /I "!CONFIRM!"=="Y" goto :EXECUTE_CLEAN
if /I "!CONFIRM!"=="N" goto :SKIP_CLEAN
if "!CONFIRM!"=="" goto :SKIP_CLEAN

echo [WARN] Invalid input. Please enter Y or N.
goto :CONFIRM_PROMPT

:: ============================================================================
:: Execute the selected clean operation
:: ============================================================================
:EXECUTE_CLEAN
set "CLEAN_ERRORS=0"

echo.
echo ============================================================
echo   Cleaning Build Artifacts
echo ============================================================
echo.

if "!CLEAN_MODE!"=="DEPS"     goto :CLEAN_DEPS_ONLY
if "!CLEAN_MODE!"=="BUILD"    goto :CLEAN_BUILD_ONLY
if "!CLEAN_MODE!"=="ALL"      goto :CLEAN_ALL
if "!CLEAN_MODE!"=="PRISTINE" goto :CLEAN_PRISTINE

:: ---------------------------------------------------------------------------
:: Mode: Build artifacts only (preserve _deps/)
:: ---------------------------------------------------------------------------
:CLEAN_BUILD_ONLY

:: Remove bin/
if exist "!BIN_DIR!" (
    echo [CLEAN] Removing: bin\
    rmdir /S /Q "!BIN_DIR!" 2>nul
    if exist "!BIN_DIR!" (
        echo [ERROR] Failed to remove: bin\  ^(files may be locked^)
        set /A "CLEAN_ERRORS+=1"
    )
)

:: Remove .vs/
if exist "!ROOT_DIR!\.vs" (
    echo [CLEAN] Removing: .vs\
    rmdir /S /Q "!ROOT_DIR!\.vs" 2>nul
    if exist "!ROOT_DIR!\.vs" (
        echo [ERROR] Failed to remove: .vs\  ^(files may be locked^)
        set /A "CLEAN_ERRORS+=1"
    )
)

:: Remove build/ contents EXCEPT _deps/
if exist "!BUILD_DIR!" (
    :: Remove files in build/ root
    echo [CLEAN] Removing: build\ contents ^(preserving _deps/^)
    for %%F in ("!BUILD_DIR!\*") do (
        del /F /Q "%%F" 2>nul
    )
    :: Remove subdirectories except _deps
    for /D %%D in ("!BUILD_DIR!\*") do (
        if /I "%%~nxD" NEQ "_deps" (
            echo [CLEAN] Removing: build\%%~nxD\
            rmdir /S /Q "%%D" 2>nul
            if exist "%%D" (
                echo [ERROR] Failed to remove: build\%%~nxD\  ^(files may be locked^)
                set /A "CLEAN_ERRORS+=1"
            )
        ) else (
            echo [SKIP]  Preserving: build\_deps\
        )
    )
)

:: Remove VS project files from root
call :CLEAN_ROOT_ARTIFACTS
goto :CLEAN_SUMMARY

:: ---------------------------------------------------------------------------
:: Mode: Third-party deps only
:: ---------------------------------------------------------------------------
:CLEAN_DEPS_ONLY

if not exist "!DEPS_DIR!" (
    echo [LOG] No dependencies directory found. Nothing to clean.
    goto :CLEAN_SUMMARY
)

:: Count what's there
set "DIR_COUNT=0"
for /D %%D in ("!DEPS_DIR!\*") do set /A "DIR_COUNT+=1"

if "!DIR_COUNT!"=="0" (
    echo [LOG] Dependencies directory is empty. Nothing to clean.
    goto :CLEAN_SUMMARY
)

:: Remove each subdirectory individually for better error reporting
for /D %%D in ("!DEPS_DIR!\*") do (
    echo [CLEAN] Removing: _deps\%%~nxD
    rmdir /S /Q "%%D" 2>nul
    if exist "%%D" (
        :: Retry with cmd /c in case of path length issues
        cmd /c "rmdir /S /Q "%%D"" 2>nul
        if exist "%%D" (
            echo [ERROR] Failed to remove: %%~nxD  ^(files may be locked^)
            set /A "CLEAN_ERRORS+=1"
        )
    )
)

:: Remove _deps directory itself if empty
dir /B "!DEPS_DIR!" 2>nul | findstr "." >nul 2>&1
if errorlevel 1 (
    rmdir /Q "!DEPS_DIR!" 2>nul
    echo [CLEAN] Removed: _deps\
)

goto :CLEAN_SUMMARY

:: ---------------------------------------------------------------------------
:: Mode: Everything (full clean)
:: ---------------------------------------------------------------------------
:CLEAN_ALL

:: Remove build/ entirely
if exist "!BUILD_DIR!" (
    echo [CLEAN] Removing: build\
    rmdir /S /Q "!BUILD_DIR!" 2>nul
    if exist "!BUILD_DIR!" (
        :: Retry with cmd /c for long path issues
        cmd /c "rmdir /S /Q "!BUILD_DIR!"" 2>nul
        if exist "!BUILD_DIR!" (
            echo [ERROR] Failed to remove: build\  ^(files may be locked^)
            set /A "CLEAN_ERRORS+=1"
        )
    )
)

:: Remove bin/
if exist "!BIN_DIR!" (
    echo [CLEAN] Removing: bin\
    rmdir /S /Q "!BIN_DIR!" 2>nul
    if exist "!BIN_DIR!" (
        echo [ERROR] Failed to remove: bin\  ^(files may be locked^)
        set /A "CLEAN_ERRORS+=1"
    )
)

:: Remove .vs/
if exist "!ROOT_DIR!\.vs" (
    echo [CLEAN] Removing: .vs\
    rmdir /S /Q "!ROOT_DIR!\.vs" 2>nul
    if exist "!ROOT_DIR!\.vs" (
        echo [ERROR] Failed to remove: .vs\  ^(files may be locked^)
        set /A "CLEAN_ERRORS+=1"
    )
)

:: Remove VS project files and CMake artifacts from root
call :CLEAN_ROOT_ARTIFACTS
goto :CLEAN_SUMMARY

:: ---------------------------------------------------------------------------
:: Mode: Pristine (nuclear — returns to freshly-cloned state)
:: ---------------------------------------------------------------------------
:CLEAN_PRISTINE

:: Remove build/ entirely
if exist "!BUILD_DIR!" (
    echo [CLEAN] Removing: build\
    rmdir /S /Q "!BUILD_DIR!" 2>nul
    if exist "!BUILD_DIR!" (
        cmd /c "rmdir /S /Q "!BUILD_DIR!"" 2>nul
        if exist "!BUILD_DIR!" (
            echo [ERROR] Failed to remove: build\  ^(files may be locked^)
            set /A "CLEAN_ERRORS+=1"
        )
    )
)

:: Remove bin/
if exist "!BIN_DIR!" (
    echo [CLEAN] Removing: bin\
    rmdir /S /Q "!BIN_DIR!" 2>nul
    if exist "!BIN_DIR!" (
        echo [ERROR] Failed to remove: bin\  ^(files may be locked^)
        set /A "CLEAN_ERRORS+=1"
    )
)

:: Remove .vs/
if exist "!ROOT_DIR!\.vs" (
    echo [CLEAN] Removing: .vs\
    rmdir /S /Q "!ROOT_DIR!\.vs" 2>nul
    if exist "!ROOT_DIR!\.vs" (
        echo [ERROR] Failed to remove: .vs\  ^(files may be locked^)
        set /A "CLEAN_ERRORS+=1"
    )
)

:: Remove logs/
if exist "!ROOT_DIR!\logs" (
    echo [CLEAN] Removing: logs\
    rmdir /S /Q "!ROOT_DIR!\logs" 2>nul
    if exist "!ROOT_DIR!\logs" (
        echo [ERROR] Failed to remove: logs\  ^(files may be locked^)
        set /A "CLEAN_ERRORS+=1"
    )
)

:: Remove VS project files and CMake artifacts from root
call :CLEAN_ROOT_ARTIFACTS
goto :CLEAN_SUMMARY

:: ---------------------------------------------------------------------------
:: Subroutine: Remove root-level VS/CMake artifacts
:: ---------------------------------------------------------------------------
:CLEAN_ROOT_ARTIFACTS
echo [CLEAN] Removing VS project files from root...
del /F /Q "!ROOT_DIR!\*.sln" 2>nul
del /F /Q "!ROOT_DIR!\*.vcxproj" 2>nul
del /F /Q "!ROOT_DIR!\*.vcxproj.filters" 2>nul
del /F /Q "!ROOT_DIR!\*.vcxproj.user" 2>nul

echo [CLEAN] Removing CMake artifacts from root...
del /F /Q "!ROOT_DIR!\CMakeCache.txt" 2>nul
del /F /Q "!ROOT_DIR!\cmake_install.cmake" 2>nul
del /F /Q "!ROOT_DIR!\Makefile" 2>nul

if exist "!ROOT_DIR!\CMakeFiles" (
    echo [CLEAN] Removing: CMakeFiles\
    rmdir /S /Q "!ROOT_DIR!\CMakeFiles" 2>nul
)
goto :EOF

:: ---------------------------------------------------------------------------
:: Summary
:: ---------------------------------------------------------------------------
:CLEAN_SUMMARY
echo.
if "!CLEAN_ERRORS!" NEQ "0" (
    echo ============================================================
    echo   [WARN] Cleanup completed with !CLEAN_ERRORS! errors.
    echo   Close any editors or processes that may have files open
    echo   and try again.
    echo ============================================================
    set "EXIT_RC=1"
) else (
    echo ============================================================
    echo   [SUCCESS] Clean complete.
    echo ============================================================
    set "EXIT_RC=0"
)

echo.
if "!CLEAN_MODE!"=="BUILD" (
    echo   Third-party deps preserved in build\_deps\.
    echo   Run GenerateProjectFiles.bat to regenerate the solution.
) else if "!CLEAN_MODE!"=="DEPS" (
    echo   Run CheckThirdParty.bat or GenerateProjectFiles.bat
    echo   to re-download dependencies ^(~64 MB, 1-3 min^).
) else if "!CLEAN_MODE!"=="ALL" (
    echo   Run Setup.bat or GenerateProjectFiles.bat to start fresh.
) else (
    echo   Project is now in a pristine state ^(as if freshly cloned^).
    echo   Run Setup.bat or GenerateProjectFiles.bat to rebuild from scratch.
)

goto :FINISH

:: ---------------------------------------------------------------------------
:: User cancelled
:: ---------------------------------------------------------------------------
:SKIP_CLEAN
echo.
echo [SKIP] Cleanup cancelled by user.
set "EXIT_RC=0"
goto :FINISH

:: ============================================================================
:: Clean exit with proper endlocal handling
:: ============================================================================
:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%"

if defined PARENT_BATCH (
    exit /B %EXIT_RC%
)

echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%

