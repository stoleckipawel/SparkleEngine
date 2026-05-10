@echo off
:: ============================================================================
:: ProjectDiscovery.bat - Shared project and target discovery helpers
:: ============================================================================
:: Internal helper module that discovers runnable projects and resolves the
:: current editor/runtime launch target names from the .sparkle-project layout.
::
:: Usage:
::   call "Internal\Projects\ProjectDiscovery.bat" ListProjects
::   call "Internal\Projects\ProjectDiscovery.bat" ResolveTargets <ProjectName|ALL> <Editor|Runtime|Both>
::
:: Outputs:
::   PROJECT_COUNT / PROJECT_<n>
::   TARGET_COUNT / TARGET_<n>
:: ============================================================================

setlocal enabledelayedexpansion

if /I "%~1"=="ListProjects" goto :LIST_PROJECTS
if /I "%~1"=="ResolveTargets" goto :RESOLVE_TARGETS

echo [ERROR] ProjectDiscovery.bat requires a valid command.
echo         Supported commands: ListProjects, ResolveTargets
set "DISCOVERY_RC=1"
goto :FINISH

:LIST_PROJECTS
call "%~dp0..\Core\Config.bat"
call :COLLECT_PROJECTS

set "EXPORTS=set PROJECT_COUNT=!PROJECT_COUNT!"
for /L %%I in (1,1,!PROJECT_COUNT!) do (
    call set "EXPORT_VALUE=%%PROJECT_%%I%%"
    set "EXPORT_VALUE=!EXPORT_VALUE: =!"
    set "EXPORTS=!EXPORTS!&set PROJECT_%%I=!EXPORT_VALUE!"
)

set "DISCOVERY_RC=0"
goto :FINISH_WITH_EXPORTS

:RESOLVE_TARGETS
call "%~dp0..\Core\Config.bat"

set "TARGET_PROJECT=%~2"
set "HOST_MODE=%~3"
if defined TARGET_PROJECT set "TARGET_PROJECT=!TARGET_PROJECT: =!"
if defined HOST_MODE set "HOST_MODE=!HOST_MODE: =!"

if "%TARGET_PROJECT%"=="" (
    echo [ERROR] ResolveTargets requires a project name or ALL.
    set "DISCOVERY_RC=1"
    goto :FINISH
)

set "VALID_HOST=0"
for %%H in (Editor Runtime Both) do (
    if /I "!HOST_MODE!"=="%%H" set "VALID_HOST=1"
)

if "!VALID_HOST!" NEQ "1" (
    echo [ERROR] Unsupported host mode '!HOST_MODE!'.
    set "DISCOVERY_RC=1"
    goto :FINISH
)

set "TARGET_COUNT=0"

if /I "!TARGET_PROJECT!"=="ALL" (
    call :COLLECT_PROJECTS
    for /L %%I in (1,1,!PROJECT_COUNT!) do (
        call set "CURRENT_PROJECT=%%PROJECT_%%I%%"
        set "CURRENT_PROJECT=!CURRENT_PROJECT: =!"
        call :ADD_PROJECT_TARGETS
    )
    goto :EXPORT_TARGETS
)

if not exist "!PROJECTS_DIR!\!TARGET_PROJECT!\.sparkle-project" (
    echo [ERROR] Project '!TARGET_PROJECT!' was not found under Projects\.
    set "DISCOVERY_RC=1"
    goto :FINISH
)

set "CURRENT_PROJECT=!TARGET_PROJECT!"
call :ADD_PROJECT_TARGETS

:EXPORT_TARGETS
set "EXPORTS=set TARGET_COUNT=!TARGET_COUNT!"
for /L %%I in (1,1,!TARGET_COUNT!) do (
    call set "EXPORT_VALUE=%%TARGET_%%I%%"
    set "EXPORT_VALUE=!EXPORT_VALUE: =!"
    set "EXPORTS=!EXPORTS!&set TARGET_%%I=!EXPORT_VALUE!"
)

set "DISCOVERY_RC=0"
goto :FINISH_WITH_EXPORTS

:COLLECT_PROJECTS
set "PROJECT_COUNT=0"
for /f "delims=" %%P in ('dir /b /ad "!PROJECTS_DIR!" 2^>nul') do (
    set "PROJ_NAME=%%P"
    set "PROJ_NAME=!PROJ_NAME: =!"
    if /I "!PROJ_NAME!" NEQ "TemplateProject" (
        if exist "!PROJECTS_DIR!\%%P\.sparkle-project" (
            set /A "PROJECT_COUNT+=1"
            set "PROJECT_!PROJECT_COUNT!=!PROJ_NAME!"
        )
    )
)
goto :EOF

:ADD_PROJECT_TARGETS
if /I "!HOST_MODE!"=="Editor" (
    set /A "TARGET_COUNT+=1"
    set "TARGET_!TARGET_COUNT!=!CURRENT_PROJECT!Editor"
    goto :EOF
)

if /I "!HOST_MODE!"=="Runtime" (
    set /A "TARGET_COUNT+=1"
    set "TARGET_!TARGET_COUNT!=!CURRENT_PROJECT!Runtime"
    goto :EOF
)

set /A "TARGET_COUNT+=1"
set "TARGET_!TARGET_COUNT!=!CURRENT_PROJECT!Editor"
set /A "TARGET_COUNT+=1"
set "TARGET_!TARGET_COUNT!=!CURRENT_PROJECT!Runtime"
goto :EOF

:FINISH_WITH_EXPORTS
set "_TMP_RC=%DISCOVERY_RC%"
endlocal & %EXPORTS%& set "PROJECT_DISCOVERY_RC=%_TMP_RC%" & exit /B %_TMP_RC%

:FINISH
set "_TMP_RC=%DISCOVERY_RC%"
endlocal & set "PROJECT_DISCOVERY_RC=%_TMP_RC%" & exit /B %_TMP_RC%
