@echo off
:: ============================================================================
:: SyncThirdParty.bat - Third-party dependency sync and repair utility
:: ============================================================================
:: Validates the build/_deps cache. When dependencies are missing or corrupt,
:: this command drives a configure pass to fetch them.
::
:: Usage: SyncThirdParty.bat
::   When PARENT_BATCH is set, syncs automatically without asking.
:: ============================================================================

setlocal enabledelayedexpansion

if not defined LOG_CAPTURED (
    call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
    exit /B %ERRORLEVEL%
)

call "%~dp0Internal\Config.bat"

set "AUTO_MODE=0"
if defined PARENT_BATCH set "AUTO_MODE=1"

set "DEP_COUNT=5"
set "DEP_1=imgui"
set "DEP_2=cgltf"
set "DEP_3=stb"
set "DEP_4=compressonator"
set "DEP_5=ktx"

set "DEP_NAME_1=Dear ImGui"
set "DEP_NAME_2=cgltf"
set "DEP_NAME_3=stb"
set "DEP_NAME_4=Compressonator"
set "DEP_NAME_5=KTX-Software"

set "DEP_SIZE_1=~7 MB"
set "DEP_SIZE_2=~1 MB"
set "DEP_SIZE_3=~5 MB"
set "DEP_SIZE_4=~5 MB"
set "DEP_SIZE_5=~46 MB"

echo.
echo ============================================================
echo   Checking Third-Party Dependencies
echo ============================================================
echo.

set "MISSING_COUNT=0"
set "PRESENT_COUNT=0"
set "CORRUPT_COUNT=0"

for /L %%I in (1,1,%DEP_COUNT%) do (
    set "DEP_ID=!DEP_%%I!"
    set "DEP_DISPLAY=!DEP_NAME_%%I!"
    set "DEP_SIZE=!DEP_SIZE_%%I!"
    set "DEP_PATH=!DEPS_DIR!\!DEP_ID!-src"

    if exist "!DEP_PATH!\*" (
        git -C "!DEP_PATH!" rev-parse --is-inside-work-tree >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] !DEP_DISPLAY! - corrupt or incomplete clone ^(!DEP_ID!-src^)
            echo         Removing corrupt directory for re-download...
            rmdir /S /Q "!DEP_PATH!" 2>nul
            if exist "!DEPS_DIR!\!DEP_ID!-subbuild" rmdir /S /Q "!DEPS_DIR!\!DEP_ID!-subbuild" 2>nul
            set /A "CORRUPT_COUNT+=1"
            set /A "MISSING_COUNT+=1"
        ) else (
            echo [OK]   !DEP_DISPLAY!
            set /A "PRESENT_COUNT+=1"
        )
    ) else (
        echo [WARN] !DEP_DISPLAY! not found  ^(!DEP_ID!-src, !DEP_SIZE!^)
        set /A "MISSING_COUNT+=1"
    )
)

echo.
if "!MISSING_COUNT!"=="0" (
    echo ============================================================
    echo   [SUCCESS] All !DEP_COUNT! third-party dependencies are present.
    echo ============================================================
    goto :TP_ALL_PRESENT
)

if "!CORRUPT_COUNT!" NEQ "0" (
    echo.
    echo [WARN] !CORRUPT_COUNT! corrupt dependencies were cleaned up.
    echo        These will be re-downloaded during sync.
)

echo ============================================================
echo   [WARN] !MISSING_COUNT! of !DEP_COUNT! dependencies are missing.
echo ============================================================
echo.
echo   Missing dependencies are fetched during GenerateSolution.bat.
echo   This requires an internet connection.
echo.
echo   Estimated download: ~64 MB total ^(shallow clones, LFS skipped^)
echo   Expected time:      1-5 minutes depending on connection
echo.

if "!AUTO_MODE!"=="1" (
    echo [LOG] Running in automated mode - proceeding with sync...
    goto :DO_SYNC
)

:SYNC_PROMPT
echo ============================================================
echo   Sync third-party dependencies now?
echo ============================================================
echo.
echo   Y^) Yes - Run GenerateSolution.bat to fetch missing dependencies
echo   N^) No  - Skip ^(build will fail without dependencies^)
echo.
echo ============================================================

set "SYNC_SEL="
set /P "SYNC_SEL=Enter choice [Y/N]: "

if /I "!SYNC_SEL!"=="Y" goto :DO_SYNC
if /I "!SYNC_SEL!"=="N" goto :SKIP_SYNC
if "!SYNC_SEL!"=="" goto :SKIP_SYNC

echo [WARN] Invalid input. Please enter Y or N.
goto :SYNC_PROMPT

:DO_SYNC
echo.
echo [LOG] Validating toolchain before sync...
set "PARENT_BATCH=1"
call "%~dp0CheckToolchain.bat" CONTINUE
set "TOOLCHAIN_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!TOOLCHAIN_RC!" NEQ "0" (
    echo [ERROR] Toolchain validation failed. Cannot sync dependencies.
    goto :SYNC_FAILED
)

echo.
echo [LOG] Running configure to fetch dependencies...
call "%~dp0Internal\CMakeHelpers.bat" Configure
set "CMAKE_RC=!ERRORLEVEL!"
if "!CMAKE_RC!" NEQ "0" (
    echo.
    echo [ERROR] Configure failed. Dependencies may not be fully synced.
    goto :SYNC_FAILED
)

echo.
echo [LOG] Verifying dependencies after sync...
echo.

set "STILL_MISSING=0"
for /L %%I in (1,1,%DEP_COUNT%) do (
    set "DEP_ID=!DEP_%%I!"
    set "DEP_DISPLAY=!DEP_NAME_%%I!"
    set "DEP_PATH=!DEPS_DIR!\!DEP_ID!-src"

    if exist "!DEP_PATH!\*" (
        git -C "!DEP_PATH!" rev-parse --is-inside-work-tree >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] !DEP_DISPLAY! - corrupt or incomplete clone after sync
            set /A "STILL_MISSING+=1"
        ) else (
            echo [OK]   !DEP_DISPLAY!
        )
    ) else (
        echo [WARN] !DEP_DISPLAY! still missing
        set /A "STILL_MISSING+=1"
    )
)

echo.
if "!STILL_MISSING!" NEQ "0" (
    echo [ERROR] !STILL_MISSING! dependencies still missing after sync.
    goto :SYNC_FAILED
)

echo ============================================================
echo   [SUCCESS] All dependencies synced successfully.
echo ============================================================
set "EXIT_RC=0"
goto :FINISH

:SYNC_FAILED
set "EXIT_RC=1"
goto :FINISH

:SKIP_SYNC
echo.
echo [SKIP] Sync declined by user. Build will fail without dependencies.
set "EXIT_RC=1"
goto :FINISH

:TP_ALL_PRESENT
set "EXIT_RC=0"
goto :FINISH

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