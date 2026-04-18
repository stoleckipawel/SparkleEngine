@echo off
:: ============================================================================
:: CookAllAssets.bat - Preferred top-level full asset cook entrypoint
:: ============================================================================
:: Runs the full offline cook flow for the selected project by invoking the
:: narrower cook commands under Scripts\Cook in order.
::
:: Usage:
::   CookAllAssets.bat <ProjectName> [Debug|Release|RelWithDebInfo]
::
:: Examples:
::   CookAllAssets.bat Showcase
::   CookAllAssets.bat Showcase Release
:: ============================================================================

setlocal enabledelayedexpansion

if not defined LOG_CAPTURED (
	call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
	set "BOOTSTRAP_RC=!ERRORLEVEL!"
	exit /B !BOOTSTRAP_RC!
)

call "%~dp0Internal\Config.bat"

set "EXIT_RC=1"
set "TARGET_PROJECT=%~1"
set "CONFIG=%~2"
set "TARGET_PROJECT=!TARGET_PROJECT: =!"
set "CONFIG=!CONFIG: =!"

if /I "%TARGET_PROJECT%"=="/h" goto :USAGE
if /I "%TARGET_PROJECT%"=="-h" goto :USAGE
if /I "%TARGET_PROJECT%"=="/help" goto :USAGE
if /I "%TARGET_PROJECT%"=="--help" goto :USAGE

call "%~dp0Internal\ProjectDiscovery.bat" ListProjects
if errorlevel 1 (
	echo [ERROR] Failed to discover runnable projects.
	set "EXIT_RC=1"
	goto :FINISH
)

if "!PROJECT_COUNT!"=="0" (
	echo [ERROR] No runnable projects found in Projects\.
	echo         Restore Projects\Showcase or add a runnable project under Projects\.
	set "EXIT_RC=1"
	goto :FINISH
)

if not defined TARGET_PROJECT goto :PROJECT_MENU
goto :VALIDATE_PROJECT

:PROJECT_MENU
if defined PARENT_BATCH goto :USAGE

echo.
echo ============================================================
echo   Select Project To Cook All Assets
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
	if "%%I"=="!PROJ_SEL!" set "TARGET_PROJECT=!PROJECT_%%I!"
)

:VALIDATE_PROJECT
set "PROJECT_FOUND=0"
for /L %%I in (1,1,!PROJECT_COUNT!) do (
	if /I "!TARGET_PROJECT!"=="!PROJECT_%%I!" set "PROJECT_FOUND=1"
)

if "!PROJECT_FOUND!"=="0" (
	echo [ERROR] Unknown project '!TARGET_PROJECT!'.
	goto :USAGE
)

if "%CONFIG%"=="" set "CONFIG=Debug"

set "VALID_CONFIG=0"
for %%C in (Debug Release RelWithDebInfo) do (
	if /I "!CONFIG!"=="%%C" set "VALID_CONFIG=1"
)

if "!VALID_CONFIG!" NEQ "1" (
	echo [ERROR] Unsupported configuration '!CONFIG!'.
	goto :USAGE
)

echo [LOG] Project: !TARGET_PROJECT!
echo [LOG] Configuration: !CONFIG!

set "PARENT_BATCH=1"

echo.
echo [LOG] Step 1/3: Cooking shader packages...
call "%~dp0Cook\CookShaders.bat" "!TARGET_PROJECT!" "!CONFIG!"
if errorlevel 1 (
	set "PARENT_BATCH="
	echo [ERROR] CookShaders step failed.
	set "EXIT_RC=1"
	goto :FINISH
)

echo.
echo [LOG] Step 2/3: Cooking texture assets...
call "%~dp0Cook\CookTextures.bat" "!TARGET_PROJECT!" "!CONFIG!"
if errorlevel 1 (
	set "PARENT_BATCH="
	echo [ERROR] CookTextures step failed.
	set "EXIT_RC=1"
	goto :FINISH
)

echo.
echo [LOG] Step 3/3: Cooking scene assets...
call "%~dp0Cook\CookAssets.bat" "!TARGET_PROJECT!" "!CONFIG!"
if errorlevel 1 (
	set "PARENT_BATCH="
	echo [ERROR] CookAssets step failed.
	set "EXIT_RC=1"
	goto :FINISH
)

set "PARENT_BATCH="

echo.
echo [SUCCESS] CookAllAssets completed successfully.
set "EXIT_RC=0"
goto :FINISH

:USAGE
echo.
echo Usage: Scripts\CookAllAssets.bat ^<ProjectName^> [Debug^|Release^|RelWithDebInfo]
echo.
echo Examples:
echo   Scripts\CookAllAssets.bat Showcase
echo   Scripts\CookAllAssets.bat Showcase Release
echo.
echo This is the single top-level full cook entrypoint in Scripts\.
echo It runs the complete offline cook flow for the selected project:
echo   1. Validate and cook shader packages
echo   2. Collect and cook texture assets
echo   3. Cook scene, mesh, and material assets
echo.
echo Narrower flows remain available under Scripts\Cook\:
echo   Scripts\Cook\CookShaders.bat
echo   Scripts\Cook\CookTextures.bat
echo   Scripts\Cook\CookAssets.bat
echo.
set "EXIT_RC=1"

:FINISH
endlocal & exit /B %EXIT_RC%