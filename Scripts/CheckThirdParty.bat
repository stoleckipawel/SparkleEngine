@echo off
:: ============================================================================
:: CheckThirdParty.bat - Third-party dependency sync validator
:: ============================================================================
:: Checks whether FetchContent dependencies have been downloaded to
:: build\_deps\. If any are missing, prompts the user to run a CMake
:: configure to fetch them automatically.
::
:: Dependencies (~64 MB total):
::   - imgui          (build\_deps\imgui-src)
::   - cgltf          (build\_deps\cgltf-src)
::   - stb            (build\_deps\stb-src)
::   - compressonator (build\_deps\compressonator-src)
::   - ktx            (build\_deps\ktx-src)
::
:: Usage: CheckThirdParty.bat
::   When PARENT_BATCH is set, syncs automatically without asking.
::   When run standalone, prompts the user interactively.
::
:: Environment:
::   PARENT_BATCH  - When set, auto-syncs and suppresses pause
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
::
:: Exit Codes:
::   0 - All dependencies present (or sync succeeded)
::   1 - Dependencies missing and user declined sync (or sync failed)
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
:: Determine mode: auto (PARENT_BATCH set) or interactive
:: ---------------------------------------------------------------------------
set "AUTO_MODE=0"
if defined PARENT_BATCH set "AUTO_MODE=1"

:: ---------------------------------------------------------------------------
:: Define expected dependencies
:: Each dep maps to a folder: build\_deps\<name>-src
:: ---------------------------------------------------------------------------
set "DEP_COUNT=5"
set "DEP_1=imgui"
set "DEP_2=cgltf"
set "DEP_3=stb"
set "DEP_4=compressonator"
set "DEP_5=ktx"

:: Friendly display names
set "DEP_NAME_1=Dear ImGui"
set "DEP_NAME_2=cgltf"
set "DEP_NAME_3=stb"
set "DEP_NAME_4=Compressonator"
set "DEP_NAME_5=KTX-Software"

:: Approximate clone sizes (shallow, LFS skipped)
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

:: ---------------------------------------------------------------------------
:: Validate each dependency
:: ---------------------------------------------------------------------------
set "MISSING_COUNT=0"
set "PRESENT_COUNT=0"
set "CORRUPT_COUNT=0"

for /L %%I in (1,1,%DEP_COUNT%) do (
    set "DEP_ID=!DEP_%%I!"
    set "DEP_DISPLAY=!DEP_NAME_%%I!"
    set "DEP_SIZE=!DEP_SIZE_%%I!"
    set "DEP_PATH=!DEPS_DIR!\!DEP_ID!-src"

    if exist "!DEP_PATH!\*" (
        :: Verify this is a valid git repo (catches interrupted clones)
        git -C "!DEP_PATH!" rev-parse --is-inside-work-tree >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] !DEP_DISPLAY! — corrupt or incomplete clone ^(!DEP_ID!-src^)
            echo         Removing corrupt directory for re-download...
            rmdir /S /Q "!DEP_PATH!" 2>nul
            :: Also remove the subbuild stamp files so FetchContent retries
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

:: ---------------------------------------------------------------------------
:: Summary
:: ---------------------------------------------------------------------------
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
echo   Dependencies are fetched automatically during CMake configure.
echo   This requires an internet connection.
echo.
echo   Estimated download: ~64 MB total ^(shallow clones, LFS skipped^)
echo   Expected time:      1-5 minutes depending on connection
echo.

:: ---------------------------------------------------------------------------
:: Prompt to sync (or auto-sync in CONTINUE mode)
:: ---------------------------------------------------------------------------
if "!AUTO_MODE!"=="1" (
    echo [LOG] Running in automated mode - proceeding with sync...
    goto :DO_SYNC
)

:SYNC_PROMPT
echo ============================================================
echo   Sync third-party dependencies now?
echo ============================================================
echo.
echo   Y^) Yes - Run CMake configure to fetch missing dependencies
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

:: ---------------------------------------------------------------------------
:: Perform sync via CMake configure
:: ---------------------------------------------------------------------------
:DO_SYNC
echo.
echo [LOG] Running CMake configure to fetch dependencies...
echo.

:: Validate required tools before attempting sync
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Cannot sync dependencies.
    echo         Install CMake and add to PATH.
    goto :SYNC_FAILED
)
where git >nul 2>&1
if errorlevel 1 (
    echo [ERROR] git not found. Cannot sync dependencies.
    echo         Install git and add to PATH.
    goto :SYNC_FAILED
)

if not exist "!BUILD_DIR!" mkdir "!BUILD_DIR!"

:: Set GIT_LFS_SKIP_SMUDGE to avoid multi-GB LFS downloads
set "GIT_LFS_SKIP_SMUDGE=1"

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

:: Unset LFS skip
set "GIT_LFS_SKIP_SMUDGE="

if "!CMAKE_RC!" NEQ "0" (
    echo.
    echo [ERROR] CMake configure failed. Dependencies may not be fully synced.
    echo         Check the output above for errors.
    goto :SYNC_FAILED
)

:: ---------------------------------------------------------------------------
:: Re-validate after sync
:: ---------------------------------------------------------------------------
echo.
echo [LOG] Verifying dependencies after sync...
echo.

set "STILL_MISSING=0"
for /L %%I in (1,1,%DEP_COUNT%) do (
    set "DEP_ID=!DEP_%%I!"
    set "DEP_DISPLAY=!DEP_NAME_%%I!"
    set "DEP_PATH=!DEPS_DIR!\!DEP_ID!-src"

    if exist "!DEP_PATH!\*" (
        :: Verify this is a valid git repo (catches interrupted clones)
        git -C "!DEP_PATH!" rev-parse --is-inside-work-tree >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] !DEP_DISPLAY! — corrupt or incomplete clone after sync
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

:: ---------------------------------------------------------------------------
:: User declined sync
:: ---------------------------------------------------------------------------
:SKIP_SYNC
echo.
echo [SKIP] Sync declined by user. Build will fail without dependencies.
set "EXIT_RC=1"
goto :FINISH

:: ---------------------------------------------------------------------------
:: All dependencies present — nothing to do
:: ---------------------------------------------------------------------------
:TP_ALL_PRESENT
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
