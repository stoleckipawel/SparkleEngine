@echo off
:: ============================================================================
:: Build.bat - Build project editor/runtime targets
:: ============================================================================
:: Builds project launch targets using the current split host model:
::   <Project>Editor, <Project>Runtime, or both.
::
:: Usage:
::   Build.bat [ProjectName|ALL] [Editor|Runtime|Both] [Debug|Release|RelWithDebInfo|All]
::
:: Examples:
::   Build.bat Showcase Editor Debug
::   Build.bat Showcase Both Release
::   Build.bat ALL Runtime RelWithDebInfo
:: ============================================================================

setlocal enabledelayedexpansion

if not defined LOG_CAPTURED (
    call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
    exit /B %ERRORLEVEL%
)

call "%~dp0Internal\Config.bat"

set "EXIT_RC=1"
set "SELECTED_PROJECT=%~1"
set "HOST_MODE=%~2"
set "CONFIG=%~3"
set "SELECTED_PROJECT=!SELECTED_PROJECT: =!"
set "HOST_MODE=!HOST_MODE: =!"
set "CONFIG=!CONFIG: =!"

if /I "%SELECTED_PROJECT%"=="/h" goto :USAGE
if /I "%SELECTED_PROJECT%"=="-h" goto :USAGE
if /I "%SELECTED_PROJECT%"=="/help" goto :USAGE
if /I "%SELECTED_PROJECT%"=="--help" goto :USAGE

call "%~dp0Internal\ProjectDiscovery.bat" ListProjects
if errorlevel 1 (
    echo [ERROR] Failed to discover runnable projects.
    goto :FINISH
)

if "!PROJECT_COUNT!"=="0" (
    echo [ERROR] No runnable projects found in Projects\.
    echo         Restore Projects\Showcase or create one using CreateProject.bat.
    goto :FINISH
)

if defined PARENT_BATCH if not defined SELECTED_PROJECT goto :USAGE
if not defined SELECTED_PROJECT goto :PROJECT_MENU
if /I "!SELECTED_PROJECT!"=="ALL" goto :HOST_MENU

set "PROJECT_FOUND=0"
for /L %%I in (1,1,!PROJECT_COUNT!) do (
    if /I "!SELECTED_PROJECT!"=="!PROJECT_%%I!" set "PROJECT_FOUND=1"
)

if "!PROJECT_FOUND!"=="0" (
    echo [ERROR] Unknown project '!SELECTED_PROJECT!'.
    goto :USAGE
)
goto :HOST_MENU

:PROJECT_MENU
echo.
echo ============================================================
echo   Select Project to Build
echo ============================================================
echo.
for /L %%I in (1,1,!PROJECT_COUNT!) do (
    call echo   %%I^) %%PROJECT_%%I%%
)
set /A "ALL_OPT=PROJECT_COUNT+1"
echo   !ALL_OPT!^) Build All Projects
echo.
echo ============================================================

set "PROJ_SEL="
set /P "PROJ_SEL=Enter choice [1-!ALL_OPT!]: "
if "!PROJ_SEL!"=="" set "PROJ_SEL=1"

set "VALID_SEL=0"
for /L %%I in (1,1,!ALL_OPT!) do (
    if "!PROJ_SEL!"=="%%I" set "VALID_SEL=1"
)
if "!VALID_SEL!"=="0" (
    echo [ERROR] Invalid selection: '!PROJ_SEL!'.
    goto :PROJECT_MENU
)

if "!PROJ_SEL!"=="!ALL_OPT!" (
    set "SELECTED_PROJECT=ALL"
) else (
    for /L %%I in (1,1,!PROJECT_COUNT!) do (
        if "%%I"=="!PROJ_SEL!" set "SELECTED_PROJECT=!PROJECT_%%I!"
    )
)

:HOST_MENU
if defined PARENT_BATCH if not defined HOST_MODE goto :USAGE
if defined HOST_MODE goto :CONFIG_MENU

echo.
echo ============================================================
echo   Select Host Target Type
echo ============================================================
echo.
echo   1^) Editor
echo   2^) Runtime
echo   3^) Both
echo.
echo ============================================================

set "HOST_SEL="
set /P "HOST_SEL=Enter choice [1-3]: "
if "!HOST_SEL!"=="" set "HOST_SEL=3"

if "!HOST_SEL!"=="1" set "HOST_MODE=Editor"
if "!HOST_SEL!"=="2" set "HOST_MODE=Runtime"
if "!HOST_SEL!"=="3" set "HOST_MODE=Both"

if not defined HOST_MODE (
    echo [ERROR] Invalid host selection.
    goto :HOST_MENU
)

:CONFIG_MENU
if defined PARENT_BATCH if not defined CONFIG goto :USAGE
if defined CONFIG goto :VALIDATE_ARGS

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

set "CONFIG_SEL="
set /P "CONFIG_SEL=Enter choice [1-4]: "
if "!CONFIG_SEL!"=="" set "CONFIG_SEL=1"

if "!CONFIG_SEL!"=="1" set "CONFIG=Debug"
if "!CONFIG_SEL!"=="2" set "CONFIG=Release"
if "!CONFIG_SEL!"=="3" set "CONFIG=RelWithDebInfo"
if "!CONFIG_SEL!"=="4" set "CONFIG=All"

if not defined CONFIG (
    echo [ERROR] Invalid configuration selection.
    goto :CONFIG_MENU
)

:VALIDATE_ARGS
set "VALID_HOST=0"
for %%H in (Editor Runtime Both) do (
    if /I "!HOST_MODE!"=="%%H" set "VALID_HOST=1"
)
if "!VALID_HOST!"=="0" (
    echo [ERROR] Unsupported host mode '!HOST_MODE!'.
    goto :USAGE
)

set "VALID_CONFIG=0"
for %%C in (Debug Release RelWithDebInfo All) do (
    if /I "!CONFIG!"=="%%C" set "VALID_CONFIG=1"
)
if "!VALID_CONFIG!"=="0" (
    echo [ERROR] Unsupported configuration '!CONFIG!'.
    goto :USAGE
)

echo.
echo [LOG] Project selection: !SELECTED_PROJECT!
echo [LOG] Host target type: !HOST_MODE!
echo [LOG] Build configuration: !CONFIG!

echo.
echo [LOG] Refreshing build files before build...
set "PARENT_BATCH=1"
call "%~dp0GenerateSolution.bat" CONTINUE
set "CONFIGURE_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!CONFIGURE_RC!" NEQ "0" (
    echo [ERROR] GenerateSolution step failed. Cannot build.
    goto :FINISH
)

call "%~dp0Internal\ProjectDiscovery.bat" ResolveTargets "!SELECTED_PROJECT!" "!HOST_MODE!"
if errorlevel 1 goto :FINISH

if "!TARGET_COUNT!"=="0" (
    echo [ERROR] No build targets resolved for '!SELECTED_PROJECT!' / '!HOST_MODE!'.
    goto :FINISH
)

set "TARGET_ARGS="
for /L %%I in (1,1,!TARGET_COUNT!) do (
    set "TARGET_ARGS=!TARGET_ARGS! !TARGET_%%I!"
)

set "HAS_SUCCESS=0"
set "EXIT_RC=0"

if /I "!CONFIG!"=="All" (
    set "RC_Debug=1"
    set "RC_Release=1"
    set "RC_RelWithDebInfo=1"

    for %%C in (Debug Release RelWithDebInfo) do (
        echo.
        echo ========================================
        echo [LOG] Building %%C targets: !TARGET_ARGS!
        echo ========================================
        call "%~dp0Internal\CMakeHelpers.bat" BuildTargets %%C !TARGET_ARGS!
        set "CFG_RC=!ERRORLEVEL!"
        set "RC_%%C=!CFG_RC!"
        if "!CFG_RC!"=="0" (
            set "HAS_SUCCESS=1"
        ) else (
            echo [ERROR] %%C build failed with code !CFG_RC!
            set "EXIT_RC=!CFG_RC!"
        )
    )
    goto :MAYBE_LAUNCH
)

echo.
echo ========================================
echo [LOG] Building targets: !TARGET_ARGS!
echo ========================================
call "%~dp0Internal\CMakeHelpers.bat" BuildTargets !CONFIG! !TARGET_ARGS!
set "EXIT_RC=!ERRORLEVEL!"
if "!EXIT_RC!"=="0" set "HAS_SUCCESS=1"

:MAYBE_LAUNCH
if "!HAS_SUCCESS!" NEQ "1" goto :FINISH
if defined PARENT_BATCH goto :FINISH
if "!TARGET_COUNT!" NEQ "1" goto :FINISH
if /I "!SELECTED_PROJECT!"=="ALL" goto :FINISH

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
if /I "!CONFIG!"=="All" (
    if "!RC_Release!"=="0" (
        set "RUN_CONFIG=Release"
    ) else if "!RC_Debug!"=="0" (
        set "RUN_CONFIG=Debug"
    ) else (
        set "RUN_CONFIG=RelWithDebInfo"
    )
)

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

:USAGE
echo.
echo Usage: Scripts\Build.bat [ProjectName^|ALL] [Editor^|Runtime^|Both] [Debug^|Release^|RelWithDebInfo^|All]
echo.
echo Examples:
echo   Scripts\Build.bat Showcase Editor Debug
echo   Scripts\Build.bat Showcase Both Release
echo   Scripts\Build.bat ALL Runtime RelWithDebInfo
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