@echo off
:: ============================================================================
:: CookAllAssets.bat - Preferred top-level full asset cook entrypoint
:: ============================================================================
:: Runs the full offline cook flow for the selected project by invoking the
:: narrower cook commands under Scripts\Cook in order.
::
:: Usage:
::   CookAllAssets.bat [ProjectName|ALL] [Debug|Release|RelWithDebInfo]
::
:: Examples:
::   CookAllAssets.bat
::   CookAllAssets.bat ALL
::   CookAllAssets.bat Showcase
::   CookAllAssets.bat Showcase Release
::
:: Manual no-argument invocation cooks all discovered runnable projects.
:: ============================================================================

setlocal enabledelayedexpansion

if not defined LOG_CAPTURED (
	call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
	set "BOOTSTRAP_RC=!ERRORLEVEL!"
	if defined PARENT_BATCH exit /B !BOOTSTRAP_RC!

	set "SUMMARY_TARGET=All discovered runnable projects"
	set "SUMMARY_CONFIG=%~2"
	if not "%~1"=="" if /I not "%~1"=="ALL" set "SUMMARY_TARGET=%~1"
	if "!SUMMARY_CONFIG!"=="" set "SUMMARY_CONFIG=Debug"

	echo.
	echo ============================================================
	if "!BOOTSTRAP_RC!"=="0" (
		echo   [SUCCESS] CookAllAssets completed successfully.
	) else (
		echo   [ERROR] CookAllAssets completed with errors.
	)
	echo   Target: !SUMMARY_TARGET!
	echo   Configuration: !SUMMARY_CONFIG!
	echo ============================================================
	echo.
	echo [LOG] Logs: %~dp0..\logs\logTools.txt
	pause
	exit /B !BOOTSTRAP_RC!
)

call "%~dp0Internal\Config.bat"

set "EXIT_RC=1"
set "TARGET_PROJECT=%~1"
set "CONFIG=%~2"
set "SUMMARY_TARGET=All discovered runnable projects"
set "FAILED_PROJECT="
if defined TARGET_PROJECT set "TARGET_PROJECT=!TARGET_PROJECT: =!"
if defined CONFIG set "CONFIG=!CONFIG: =!"

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

if "%CONFIG%"=="" set "CONFIG=Debug"
if defined TARGET_PROJECT if /I not "!TARGET_PROJECT!"=="ALL" set "SUMMARY_TARGET=!TARGET_PROJECT!"

set "VALID_CONFIG=0"
for %%C in (Debug Release RelWithDebInfo) do (
	if /I "!CONFIG!"=="%%C" set "VALID_CONFIG=1"
)

if "!VALID_CONFIG!" NEQ "1" (
	echo [ERROR] Unsupported configuration '!CONFIG!'.
	goto :USAGE
)

if not defined TARGET_PROJECT goto :COOK_ALL_PROJECTS
if /I "!TARGET_PROJECT!"=="ALL" goto :COOK_ALL_PROJECTS
goto :VALIDATE_PROJECT

:COOK_ALL_PROJECTS
echo [LOG] No project selection provided. Cooking all discovered runnable projects.
echo [LOG] Configuration: !CONFIG!
echo [LOG] Projects discovered: !PROJECT_COUNT!

set "COOKED_PROJECT_COUNT=0"
for /L %%I in (1,1,!PROJECT_COUNT!) do (
	call set "TARGET_PROJECT=%%PROJECT_%%I%%"
	echo.
	echo ============================================================
	echo [LOG] Full cook %%I/!PROJECT_COUNT!: !TARGET_PROJECT!
	echo ============================================================
	call :RUN_FULL_COOK "!TARGET_PROJECT!" "!CONFIG!"
	if errorlevel 1 (
		set "FAILED_PROJECT=!TARGET_PROJECT!"
		echo [ERROR] Full cook failed for project '!TARGET_PROJECT!'.
		set "EXIT_RC=1"
		goto :FINISH
	)
	set /A "COOKED_PROJECT_COUNT+=1"
)

echo.
echo [SUCCESS] CookAllAssets completed successfully for !COOKED_PROJECT_COUNT! discovered project(s).
set "EXIT_RC=0"
goto :FINISH

:VALIDATE_PROJECT
set "PROJECT_FOUND=0"
for /L %%I in (1,1,!PROJECT_COUNT!) do (
	if /I "!TARGET_PROJECT!"=="!PROJECT_%%I!" set "PROJECT_FOUND=1"
)

if "!PROJECT_FOUND!"=="0" (
	echo [ERROR] Unknown project '!TARGET_PROJECT!'.
	goto :USAGE
)

call :RUN_FULL_COOK "!TARGET_PROJECT!" "!CONFIG!"
if errorlevel 1 (
	set "FAILED_PROJECT=!TARGET_PROJECT!"
	set "EXIT_RC=1"
	goto :FINISH
)

echo.
echo [SUCCESS] CookAllAssets completed successfully.
set "EXIT_RC=0"
goto :FINISH

:RUN_FULL_COOK
set "TARGET_PROJECT=%~1"
set "CONFIG=%~2"

echo [LOG] Project: !TARGET_PROJECT!
echo [LOG] Configuration: !CONFIG!

set "PARENT_BATCH=1"

echo.
echo [LOG] Step 1/3: Cooking shader packages...
call "%~dp0Cook\CookShaders.bat" "!TARGET_PROJECT!" "!CONFIG!"
if errorlevel 1 (
	set "PARENT_BATCH="
	echo [ERROR] CookShaders step failed.
	exit /B 1
)

echo.
echo [LOG] Step 2/3: Cooking texture assets...
call "%~dp0Cook\CookTextures.bat" "!TARGET_PROJECT!" "!CONFIG!"
if errorlevel 1 (
	set "PARENT_BATCH="
	echo [ERROR] CookTextures step failed.
	exit /B 1
)

echo.
echo [LOG] Step 3/3: Cooking scene assets...
call "%~dp0Cook\CookAssets.bat" "!TARGET_PROJECT!" "!CONFIG!"
if errorlevel 1 (
	set "PARENT_BATCH="
	echo [ERROR] CookAssets step failed.
	exit /B 1
)

set "PARENT_BATCH="
exit /B 0

:USAGE
echo.
echo Usage: Scripts\CookAllAssets.bat [^<ProjectName^>^|ALL] [Debug^|Release^|RelWithDebInfo]
echo.
echo Examples:
echo   Scripts\CookAllAssets.bat
echo   Scripts\CookAllAssets.bat ALL
echo   Scripts\CookAllAssets.bat Showcase
echo   Scripts\CookAllAssets.bat Showcase Release
echo.
echo This is the single top-level full cook entrypoint in Scripts\.
echo With no project argument, it cooks all discovered runnable projects.
echo When a project is specified, it runs the complete offline cook flow for that project:
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