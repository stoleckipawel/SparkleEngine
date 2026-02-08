@echo off
:: ============================================================================
:: BuildProjects.bat - Interactive project build dispatcher
:: ============================================================================
:: Prompts for project selection and build configuration, then builds.
:: Automatically generates VS solution if not present.
::
:: Usage: BuildProjects.bat
::   Interactive menu allows selection of specific project or all projects,
::   then configuration: Debug, Release, RelWithDebInfo, All
::
:: Environment:
::   PARENT_BATCH  - When set, suppresses interactive prompts and pause
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
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
:: Step 0: Check third-party dependencies are synced
:: ---------------------------------------------------------------------------
echo [LOG] Checking third-party dependencies...
set "PARENT_BATCH=1"
call "%~dp0CheckThirdParty.bat"
set "TP_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!TP_RC!" NEQ "0" (
    echo [ERROR] Third-party dependency check failed.
    set "OVERALL_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Discover available projects (with .sparkle-project marker, excluding TemplateProject)
:: ---------------------------------------------------------------------------
set "PROJECT_COUNT=0"
for /D %%P in ("!PROJECTS_DIR!\*") do (
    set "PROJ_NAME=%%~nxP"
    if /I "!PROJ_NAME!" NEQ "TemplateProject" (
        if exist "%%P\.sparkle-project" (
            set /A "PROJECT_COUNT+=1"
            set "PROJECT_!PROJECT_COUNT!=!PROJ_NAME!"
        )
    )
)

if "!PROJECT_COUNT!"=="0" (
    echo [ERROR] No projects found in Projects\ folder.
    echo         Create a project using CreateNewProject.bat
    set "OVERALL_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Project selection menu
:: ---------------------------------------------------------------------------
:PROJECT_MENU
echo.
echo ============================================================
echo   Select Project to Build
echo ============================================================
echo.

:: List individual projects first (1, 2, 3, ...)
for /L %%I in (1,1,!PROJECT_COUNT!) do (
    call echo   %%I^) %%PROJECT_%%I%%
)

:: "Build All" is the last option
set /A "ALL_OPT=PROJECT_COUNT+1"
echo   !ALL_OPT!^) Build All Projects
echo.
echo ============================================================

set "PROJ_SEL="
set /P "PROJ_SEL=Enter choice [1-!ALL_OPT!]: "

:: Default to first project if empty
if "!PROJ_SEL!"=="" set "PROJ_SEL=1"

:: Validate selection
set "VALID_SEL=0"
for /L %%I in (1,1,!ALL_OPT!) do (
    if "!PROJ_SEL!"=="%%I" set "VALID_SEL=1"
)
if "!VALID_SEL!"=="0" (
    echo.
    echo [ERROR] Invalid selection: '!PROJ_SEL!' - Please enter 1 to !ALL_OPT!.
    goto PROJECT_MENU
)

:: Map selection to project name
set "SELECTED_PROJECT="
if "!PROJ_SEL!"=="!ALL_OPT!" (
    set "SELECTED_PROJECT=ALL"
    echo.
    echo [LOG] Selected: Build All Projects
) else (
    for /L %%I in (1,1,!PROJECT_COUNT!) do (
        if "%%I"=="!PROJ_SEL!" set "SELECTED_PROJECT=!PROJECT_%%I!"
    )
    echo.
    echo [LOG] Selected: !SELECTED_PROJECT!
)

:: ---------------------------------------------------------------------------
:: Configuration selection menu
:: ---------------------------------------------------------------------------
:CONFIG_MENU
echo.
echo ============================================================
echo   Select Build Configuration
echo ============================================================
echo.
echo   1^) Debug
echo   2^) Release
echo   3^) RelWithDebInfo
echo   4^) All Configurations
echo.
echo ============================================================

set "SEL="
set /P "SEL=Enter choice [1-4]: "

:: Default to Debug if empty
if "!SEL!"=="" set "SEL=1"

:: Map selection to configuration name
set "CONFIG="
if "!SEL!"=="1" set "CONFIG=Debug"
if "!SEL!"=="2" set "CONFIG=Release"
if "!SEL!"=="3" set "CONFIG=RelWithDebInfo"
if "!SEL!"=="4" set "CONFIG=All"

if not defined CONFIG (
    echo.
    echo [ERROR] Invalid selection: '!SEL!' - Please enter 1, 2, 3, or 4.
    goto CONFIG_MENU
)

echo.
echo [LOG] Selected configuration: !CONFIG!

:: Validate internal implementation exists
set "IMPL_SCRIPT=%~dp0Internal\BuildProjectsImpl.bat"
if not exist "!IMPL_SCRIPT!" (
    echo [ERROR] Missing: Scripts\Internal\BuildProjectsImpl.bat
    set "OVERALL_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Execute build(s)
:: ---------------------------------------------------------------------------
set "PARENT_BATCH=1"
set "HAS_SUCCESS=0"
set "OVERALL_RC=0"

if /I "!CONFIG!"=="All" (
    :: Track per-configuration results for launch prompt
    set "RC_Debug=1"
    set "RC_Release=1"
    set "RC_RelWithDebInfo=1"
    
    for %%C in (Debug Release RelWithDebInfo) do (
        echo.
        echo ========================================
        echo [LOG] Building: %%C
        echo ========================================
        call "!IMPL_SCRIPT!" %%C "!SELECTED_PROJECT!"
        set "CFG_RC=!ERRORLEVEL!"
        set "RC_%%C=!CFG_RC!"
        
        if "!CFG_RC!"=="0" (
            set "HAS_SUCCESS=1"
        ) else (
            echo [ERROR] %%C build failed with code !CFG_RC!
            set "OVERALL_RC=!CFG_RC!"
        )
    )
) else (
    call "!IMPL_SCRIPT!" !CONFIG! "!SELECTED_PROJECT!"
    set "OVERALL_RC=!ERRORLEVEL!"
    if "!OVERALL_RC!"=="0" set "HAS_SUCCESS=1"
)

set "PARENT_BATCH="

:: ---------------------------------------------------------------------------
:: Optional: Launch built executable
:: ---------------------------------------------------------------------------
if "!HAS_SUCCESS!"=="1" (
    echo.
    echo ============================================================
    echo   Launch Executable?
    echo ============================================================
    echo.
    echo   Y^) Yes - Launch the built executable
    echo   N^) No  - Exit without launching
    echo.
    echo ============================================================
    
    :LAUNCH_PROMPT
    set "LAUNCH_SEL="
    set /P "LAUNCH_SEL=Enter choice [Y/N]: "
    
    if /I "!LAUNCH_SEL!"=="Y" goto :DO_LAUNCH
    if /I "!LAUNCH_SEL!"=="N" goto :SKIP_LAUNCH
    if "!LAUNCH_SEL!"=="" goto :SKIP_LAUNCH
    
    echo [WARN] Invalid input. Please enter Y or N.
    goto :LAUNCH_PROMPT
)
goto :SKIP_LAUNCH

:DO_LAUNCH
:: Determine output directory based on configuration
set "RUN_CONFIG=!CONFIG!"
if /I "!CONFIG!"=="All" (
    :: Prefer Release, fall back to Debug if Release failed
    set "RUN_CONFIG=Release"
    if "!RC_Release!" NEQ "0" if "!RC_Debug!"=="0" set "RUN_CONFIG=Debug"
)

set "LAUNCH_BIN_DIR=!BIN_DIR!\!RUN_CONFIG!"
if not exist "!LAUNCH_BIN_DIR!" (
    echo [WARN] Output directory not found: !LAUNCH_BIN_DIR!
    goto :SKIP_LAUNCH
)

:: Find executable matching the selected project
set "TARGET_EXE="
if "!SELECTED_PROJECT!"=="ALL" (
    :: For "All" builds, find first executable
    for %%F in ("!LAUNCH_BIN_DIR!\*.exe") do (
        if not defined TARGET_EXE set "TARGET_EXE=%%~fF"
    )
) else (
    :: Look for specific project executable
    if exist "!LAUNCH_BIN_DIR!\!SELECTED_PROJECT!.exe" (
        set "TARGET_EXE=!LAUNCH_BIN_DIR!\!SELECTED_PROJECT!.exe"
    ) else (
        :: Fallback to first exe
        for %%F in ("!LAUNCH_BIN_DIR!\*.exe") do (
            if not defined TARGET_EXE set "TARGET_EXE=%%~fF"
        )
    )
)

if not defined TARGET_EXE (
    echo [WARN] No executables found in: !LAUNCH_BIN_DIR!
    goto :SKIP_LAUNCH
)

:: Launch with working directory set to bin folder (for imgui.ini, etc.)
echo.
echo [LOG] Launching: !TARGET_EXE!
start "" /D "!LAUNCH_BIN_DIR!" "!TARGET_EXE!"

:SKIP_LAUNCH
echo.
echo [LOG] BuildProjects.bat completed.
goto :FINISH

:: ============================================================================
:: Clean exit with proper endlocal handling
:: ============================================================================
:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%OVERALL_RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%"

if defined PARENT_BATCH (
    exit /B %EXIT_RC%
)

echo.
echo ============================================================
if "%EXIT_RC%"=="0" (
    echo   [SUCCESS] Build completed successfully.
) else (
    echo   [ERROR] Build completed with errors.
)
echo ============================================================
echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%

