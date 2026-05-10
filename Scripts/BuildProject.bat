@echo off
:: ============================================================================
:: BuildProject.bat - Build one project editor/runtime targets
:: ============================================================================
:: Builds one project's launch target using the Unreal-style Sparkle profile:
::   *Editor profiles build <Project>Editor.
::   *Game profiles build <Project>Runtime.
::
:: Usage:
::   BuildProject.bat <ProjectName> [DebugEditor|DebugGame|DevelopmentEditor|DevelopmentGame|ShippingEditor|ShippingGame]
::
:: Examples:
::   BuildProject.bat Showcase DevelopmentEditor
::   BuildProject.bat Showcase ShippingGame
:: ============================================================================

setlocal enabledelayedexpansion

if not defined LOG_CAPTURED (
    call "%~dp0Internal\Core\BootstrapLog.bat" "%~f0" %*
    set "BOOTSTRAP_RC=!ERRORLEVEL!"
    exit /B !BOOTSTRAP_RC!
)

call "%~dp0Internal\Core\Config.bat"

set "EXIT_RC=1"
set "SELECTED_PROJECT=%~1"
set "CONFIG=%~2"
if defined SELECTED_PROJECT set "SELECTED_PROJECT=!SELECTED_PROJECT: =!"
if defined CONFIG set "CONFIG=!CONFIG: =!"

if /I "%SELECTED_PROJECT%"=="/h" goto :USAGE
if /I "%SELECTED_PROJECT%"=="-h" goto :USAGE
if /I "%SELECTED_PROJECT%"=="/help" goto :USAGE
if /I "%SELECTED_PROJECT%"=="--help" goto :USAGE

call "%~dp0Internal\Projects\ProjectDiscovery.bat" ListProjects
if errorlevel 1 (
    echo [ERROR] Failed to discover runnable projects.
    goto :FINISH
)

if "!PROJECT_COUNT!"=="0" (
    echo [ERROR] No runnable projects found in Projects\.
    echo         Restore Projects\Showcase or add a runnable project under Projects\.
    goto :FINISH
)

if defined PARENT_BATCH if not defined SELECTED_PROJECT goto :USAGE
if not defined SELECTED_PROJECT goto :PROJECT_MENU

set "PROJECT_FOUND=0"
for /L %%I in (1,1,!PROJECT_COUNT!) do (
    if /I "!SELECTED_PROJECT!"=="!PROJECT_%%I!" set "PROJECT_FOUND=1"
)

if "!PROJECT_FOUND!"=="0" (
    echo [ERROR] Unknown project '!SELECTED_PROJECT!'.
    goto :USAGE
)
goto :CONFIG_MENU

:PROJECT_MENU
echo.
echo ============================================================
echo   Select Project to Build
echo ============================================================
echo.
for /L %%I in (1,1,!PROJECT_COUNT!) do (
    call echo   %%I^) %%PROJECT_%%I%%
)
echo.
echo ============================================================

set "PROJ_SEL="
set /P "PROJ_SEL=Enter choice [1-!PROJECT_COUNT!]: "
if "!PROJ_SEL!"=="" set "PROJ_SEL=1"

set "VALID_SEL=0"
for /L %%I in (1,1,!PROJECT_COUNT!) do (
    if "!PROJ_SEL!"=="%%I" set "VALID_SEL=1"
)
if "!VALID_SEL!"=="0" (
    echo [ERROR] Invalid selection: '!PROJ_SEL!'.
    goto :PROJECT_MENU
)

for /L %%I in (1,1,!PROJECT_COUNT!) do (
    if "%%I"=="!PROJ_SEL!" set "SELECTED_PROJECT=!PROJECT_%%I!"
)

:CONFIG_MENU
if defined PARENT_BATCH if not defined CONFIG goto :USAGE
if defined CONFIG goto :VALIDATE_ARGS

echo.
echo ============================================================
echo   Select Build Configuration
echo ============================================================
echo.
echo   1^) DebugEditor
echo   2^) DebugGame
echo   3^) DevelopmentEditor
echo   4^) DevelopmentGame
echo   5^) ShippingEditor
echo   6^) ShippingGame
echo.
echo ============================================================

set "CONFIG_SEL="
set /P "CONFIG_SEL=Enter choice [1-6]: "
if "!CONFIG_SEL!"=="" set "CONFIG_SEL=3"

if "!CONFIG_SEL!"=="1" set "CONFIG=DebugEditor"
if "!CONFIG_SEL!"=="2" set "CONFIG=DebugGame"
if "!CONFIG_SEL!"=="3" set "CONFIG=DevelopmentEditor"
if "!CONFIG_SEL!"=="4" set "CONFIG=DevelopmentGame"
if "!CONFIG_SEL!"=="5" set "CONFIG=ShippingEditor"
if "!CONFIG_SEL!"=="6" set "CONFIG=ShippingGame"

if not defined CONFIG (
    echo [ERROR] Invalid configuration selection.
    goto :CONFIG_MENU
)

:VALIDATE_ARGS
set "VALID_CONFIG=0"
for %%C in (DebugEditor DebugGame DevelopmentEditor DevelopmentGame ShippingEditor ShippingGame) do (
    if /I "!CONFIG!"=="%%C" (
        set "CONFIG=%%C"
        set "VALID_CONFIG=1"
    )
)
if "!VALID_CONFIG!"=="0" (
    echo [ERROR] Unsupported build profile '!CONFIG!'.
    goto :USAGE
)

echo.
echo [LOG] Project selection: !SELECTED_PROJECT!
echo [LOG] Build profile: !CONFIG!

echo.
echo [LOG] Ensuring build files are current...
call "%~dp0Internal\Build\EnsureBuildFiles.bat"
set "ENSURE_RC=!ERRORLEVEL!"
if "!ENSURE_RC!" NEQ "0" (
    echo [ERROR] Build-file preparation failed. Cannot build.
    goto :FINISH
)

set "HAS_SUCCESS=0"
set "EXIT_RC=0"

call :BUILD_PROFILE !CONFIG!
set "EXIT_RC=!ERRORLEVEL!"
if "!EXIT_RC!"=="0" set "HAS_SUCCESS=1"

:MAYBE_LAUNCH
if "!HAS_SUCCESS!" NEQ "1" goto :FINISH
if defined PARENT_BATCH goto :FINISH
if "!TARGET_COUNT!" NEQ "1" goto :FINISH

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
if /I "!LAUNCH_SEL!"=="N" goto :FINISH
if "!LAUNCH_SEL!"=="" goto :FINISH

echo [WARN] Invalid input. Please enter Y or N.
goto :LAUNCH_PROMPT

:DO_LAUNCH
set "RUN_CONFIG=!CONFIG!"
set "TARGET_EXE=!BIN_DIR!\!RUN_CONFIG!\!TARGET_1!.exe"
set "PROJECT_WORKDIR=!PROJECTS_DIR!\!SELECTED_PROJECT!"

if not exist "!TARGET_EXE!" (
    echo [WARN] Expected executable was not found: !TARGET_EXE!
    goto :FINISH
)

if not exist "!PROJECT_WORKDIR!\.sparkle-project" (
    echo [WARN] Expected project working directory was not found: !PROJECT_WORKDIR!
    goto :FINISH
)

echo [LOG] Launching: !TARGET_EXE!
echo [LOG] Working directory: !PROJECT_WORKDIR!
start "" /D "!PROJECT_WORKDIR!" "!TARGET_EXE!"
goto :FINISH

:BUILD_PROFILE
set "CURRENT_PROFILE=%~1"
call :RESOLVE_PROFILE_TARGETS "!CURRENT_PROFILE!"
if errorlevel 1 exit /B 1

echo.
echo ========================================
echo [LOG] Building !CURRENT_PROFILE! targets: !TARGET_ARGS!
echo ========================================
call "%~dp0Internal\Build\CMakeHelpers.bat" BuildTargets !CURRENT_PROFILE! !TARGET_ARGS!
exit /B !ERRORLEVEL!

:RESOLVE_PROFILE_TARGETS
set "PROFILE_TO_RESOLVE=%~1"
set "HOST_MODE="
if /I "!PROFILE_TO_RESOLVE:~-6!"=="Editor" set "HOST_MODE=Editor"
if /I "!PROFILE_TO_RESOLVE:~-4!"=="Game" set "HOST_MODE=Runtime"

if not defined HOST_MODE (
    echo [ERROR] Build profile '!PROFILE_TO_RESOLVE!' does not end in Editor or Game.
    exit /B 1
)

call "%~dp0Internal\Projects\ProjectDiscovery.bat" ResolveTargets "!SELECTED_PROJECT!" "!HOST_MODE!"
if errorlevel 1 exit /B 1

if "!TARGET_COUNT!"=="0" (
    echo [ERROR] No build targets resolved for '!SELECTED_PROJECT!' / '!HOST_MODE!'.
    exit /B 1
)

set "TARGET_ARGS="
for /L %%I in (1,1,!TARGET_COUNT!) do (
    set "TARGET_ARGS=!TARGET_ARGS! !TARGET_%%I!"
)
exit /B 0

:USAGE
echo.
echo Usage: Scripts\BuildProject.bat ^<ProjectName^> [DebugEditor^|DebugGame^|DevelopmentEditor^|DevelopmentGame^|ShippingEditor^|ShippingGame]
echo.
echo Examples:
echo   Scripts\BuildProject.bat Showcase DevelopmentEditor
echo   Scripts\BuildProject.bat Showcase ShippingGame
echo.
echo You must provide or select one specific project.
echo.
set "EXIT_RC=1"

:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
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
