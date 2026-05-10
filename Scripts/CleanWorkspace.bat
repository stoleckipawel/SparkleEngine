@echo off
:: ============================================================================
:: CleanWorkspace.bat - Unified build artifact cleanup utility
:: ============================================================================
:: Removes generated build artifacts with fine-grained control:
::   1) Build artifacts only - build/ (except _deps/), project-local build/, .vs/
::   2) Third-party deps only - build/_deps/
::   3) Everything - full clean (build/, project-local generated output, .vs/ including _deps/)
::   4) Pristine - tracked repo state (build/, project-local generated output, .vs/, .vscode/, logs/)
::
:: Usage: CleanWorkspace.bat [BUILD|DEPS|ALL|PRISTINE]
::   BUILD    - Remove build artifacts only (preserve third-party deps)
::   DEPS     - Remove third-party dependencies only
::   ALL      - Remove everything (build + deps + generated project output)
::   PRISTINE - Remove generated outputs and return to tracked repo state
::   (no arg) - Show interactive menu
::
:: Environment:
::   PARENT_BATCH  - When set, suppresses pause on completion
::
:: Exit Codes:
::   0 - Cleanup completed (or nothing to clean, or user cancelled)
::   1 - Cleanup failed (locked files)
:: ============================================================================

setlocal enabledelayedexpansion

:: ---------------------------------------------------------------------------
:: CleanWorkspace intentionally does not bootstrap logging. PRISTINE removes the
:: logs directory, so creating a log file inside that tree would lock the very
:: artifact this script is expected to delete.
:: ---------------------------------------------------------------------------

:: ---------------------------------------------------------------------------
:: Load shared configuration
:: ---------------------------------------------------------------------------
call "%~dp0Internal\Core\Config.bat"

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
echo   1^) Build artifacts only   ^(build/, Projects/*/build/, .vs/^)
echo      Preserves third-party deps in build\_deps\.
echo.
echo   2^) Third-party deps only  ^(build/_deps/, ~64 MB^)
echo      Re-download required on next cmake configure.
echo.
echo   3^) Everything             ^(build + deps + project-local generated output^)
echo      Full rebuild required after this.
echo.
echo   4^) Pristine               ^(tracked repo state^)
echo      Removes build/, logs/, imgui.ini, project-local generated output, .vs/, .vscode/
echo      and keeps tracked project assets.
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
    echo       Will remove: build/ ^(except _deps/^), Projects/*/build/, .vs/
) else if "!CLEAN_MODE!"=="DEPS" (
    echo [LOG] Selected: Third-party deps only
    echo       Will remove: build/_deps/
    echo       Re-download: ~64 MB, 1-3 min to re-sync
) else if "!CLEAN_MODE!"=="ALL" (
    echo [LOG] Selected: Everything
    echo       Will remove: build/, project-local build/log/imgui.ini, root imgui.ini, .vs/
    echo       Includes all third-party deps ^(~64 MB re-download^)
) else (
    echo [LOG] Selected: Pristine ^(tracked repo state^)
    echo       Will remove: build/, logs/, imgui.ini, project-local build/log/imgui.ini, .vs/, .vscode/
    echo       Will keep tracked project assets, including committed cooked content.
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
echo   Cleaning Workspace Artifacts
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
call :REMOVE_DIR "!ROOT_DIR!\.vs" ".vs\"
call :CLEAN_PROJECT_GENERATED_OUTPUTS BUILD

:: Remove build/ contents EXCEPT _deps/
if exist "!BUILD_DIR!" (
    echo [CLEAN] Removing: build\ contents ^(preserving _deps/^)
    for %%F in ("!BUILD_DIR!\*") do (
        del /F /Q "%%F" 2>nul
    )
    for /D %%D in ("!BUILD_DIR!\*") do (
        if /I "%%~nxD" NEQ "_deps" (
            call :REMOVE_DIR "%%D" "build\%%~nxD\"
        ) else (
            echo [SKIP]  Preserving: build\_deps\
        )
    )
)

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
    call :REMOVE_DIR "%%D" "_deps\%%~nxD"
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
call :REMOVE_DIR "!BUILD_DIR!" "build\"
call :REMOVE_DIR "!ROOT_DIR!\.vs" ".vs\"
call :CLEAN_PROJECT_GENERATED_OUTPUTS PRISTINE
call :REMOVE_FILE "!ROOT_DIR!\imgui.ini" "imgui.ini"
call :CLEAN_ROOT_ARTIFACTS
goto :CLEAN_SUMMARY

:: ---------------------------------------------------------------------------
:: Mode: Pristine (generated outputs only - returns to tracked repo state)
:: ---------------------------------------------------------------------------
:CLEAN_PRISTINE
call :REMOVE_DIR "!BUILD_DIR!" "build\"
call :REMOVE_DIR "!ROOT_DIR!\.vs" ".vs\"
call :REMOVE_DIR "!ROOT_DIR!\.vscode" ".vscode\"
call :REMOVE_DIR "!ROOT_DIR!\logs" "logs\"
call :CLEAN_PROJECT_GENERATED_OUTPUTS PRISTINE
call :REMOVE_FILE "!ROOT_DIR!\imgui.ini" "imgui.ini"
call :CLEAN_ROOT_ARTIFACTS
goto :CLEAN_SUMMARY

:: ============================================================================
:: Subroutine: Remove project-local generated outputs
:: ============================================================================
:: Args: %1 = BUILD or PRISTINE
:CLEAN_PROJECT_GENERATED_OUTPUTS
if not exist "!PROJECTS_DIR!" goto :EOF

for /D %%P in ("!PROJECTS_DIR!\*") do (
    call :REMOVE_DIR "%%~fP\build" "Projects\%%~nxP\build\"
    if /I "%~1"=="PRISTINE" (
        call :REMOVE_DIR "%%~fP\logs" "Projects\%%~nxP\logs\"
        call :REMOVE_FILE "%%~fP\imgui.ini" "Projects\%%~nxP\imgui.ini"
    )
)
goto :EOF

:: ============================================================================
:: Subroutine: Safely remove a directory with retry
:: ============================================================================
:: Args: %1 = absolute path, %2 = display name for logging
:: Increments CLEAN_ERRORS on failure (locked files, permission issues).
:REMOVE_DIR
if not exist "%~1" goto :EOF

echo [CLEAN] Removing: %~2
rmdir /S /Q "%~1" 2>nul
if not exist "%~1" goto :EOF

:: Retry with the PowerShell helper for trees that cmd/rmdir fails to remove cleanly.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Internal\Utilities\RemoveDirectory.ps1" -TargetPath "%~1" >nul 2>&1
if exist "%~1" (
    echo [ERROR] Failed to remove: %~2  ^(files may be locked^)
    set /A "CLEAN_ERRORS+=1"
)
goto :EOF

:: ============================================================================
:: Subroutine: Safely remove a generated file
:: ============================================================================
:: Args: %1 = absolute path, %2 = display name for logging
:REMOVE_FILE
if not exist "%~1" goto :EOF

echo [CLEAN] Removing: %~2
del /F /Q "%~1" 2>nul
if not exist "%~1" goto :EOF

powershell -NoProfile -ExecutionPolicy Bypass -Command "Remove-Item -LiteralPath '%~1' -Force -ErrorAction SilentlyContinue" >nul 2>&1
if exist "%~1" (
    echo [ERROR] Failed to remove: %~2  ^(file may be locked^)
    set /A "CLEAN_ERRORS+=1"
)
goto :EOF

:: ============================================================================
:: Subroutine: Remove root-level VS/CMake artifacts
:: ============================================================================
:: These should not exist when using an out-of-source build, but CMake
:: sometimes creates them if accidentally invoked from the repo root.
:CLEAN_ROOT_ARTIFACTS
echo [CLEAN] Removing VS project files from root...
del /F /Q "!ROOT_DIR!\*.sln" 2>nul
del /F /Q "!ROOT_DIR!\*.slnx" 2>nul
del /F /Q "!ROOT_DIR!\*.vcxproj" 2>nul
del /F /Q "!ROOT_DIR!\*.vcxproj.filters" 2>nul
del /F /Q "!ROOT_DIR!\*.vcxproj.user" 2>nul

echo [CLEAN] Removing CMake artifacts from root...
del /F /Q "!ROOT_DIR!\CMakeCache.txt" 2>nul
del /F /Q "!ROOT_DIR!\cmake_install.cmake" 2>nul
del /F /Q "!ROOT_DIR!\Makefile" 2>nul

call :REMOVE_DIR "!ROOT_DIR!\CMakeFiles" "CMakeFiles\"
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
    echo   Run GenerateSolution.bat to regenerate the build files.
) else if "!CLEAN_MODE!"=="DEPS" (
    echo   Run GenerateSolution.bat
    echo   to re-download dependencies ^(~64 MB, 1-3 min^).
) else if "!CLEAN_MODE!"=="ALL" (
    echo   Run SetupWorkspace.bat or GenerateSolution.bat to start fresh.
) else (
    echo   Project is now back to the tracked repo state.
    echo   Tracked project assets, including committed cooked content, were preserved.
    echo   Run SetupWorkspace.bat or GenerateSolution.bat to rebuild from scratch.
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

if defined PARENT_BATCH exit /B %EXIT_RC%

pause
exit /B %EXIT_RC%
