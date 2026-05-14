@echo off
:: ============================================================================
:: CookAllAssets.bat - Preferred top-level full asset cook entrypoint
:: ============================================================================
:: Launch entrypoint for the AssetCooker project planner.
::
:: Usage:
::   CookAllAssets.bat [ProjectName|ALL] [DebugEditor|DebugGame|DevelopmentEditor|DevelopmentGame|ShippingEditor|ShippingGame]
:: ============================================================================

setlocal enabledelayedexpansion

set "INTERACTIVE=1"
if defined PARENT_BATCH set "INTERACTIVE=0"

if not defined LOG_CAPTURED (
	call "%~dp0Internal\Core\BootstrapLog.bat" "%~f0" %*
	set "BOOTSTRAP_RC=!ERRORLEVEL!"
	exit /B !BOOTSTRAP_RC!
)

call "%~dp0Internal\Core\Config.bat"

set "EXIT_RC=1"
set "TARGET_PROJECT=%~1"
set "CONFIG=%~2"
set "COOK_MODE="
set "CLEAN_COOKED_PATH="
if defined TARGET_PROJECT set "TARGET_PROJECT=!TARGET_PROJECT: =!"
if defined CONFIG set "CONFIG=!CONFIG: =!"

if /I "%TARGET_PROJECT%"=="/h" goto :USAGE
if /I "%TARGET_PROJECT%"=="-h" goto :USAGE
if /I "%TARGET_PROJECT%"=="/help" goto :USAGE
if /I "%TARGET_PROJECT%"=="--help" goto :USAGE

if not defined TARGET_PROJECT set "TARGET_PROJECT=ALL"
if "%CONFIG%"=="" set "CONFIG=DevelopmentGame"

echo.
echo [LOG] Asset cook request:
echo [LOG]   Target project: !TARGET_PROJECT!
echo [LOG]   Configuration: !CONFIG!
echo [LOG]   Repository: !ROOT_DIR!

call :CHOOSE_COOK_MODE
if errorlevel 1 (
	set "EXIT_RC=1"
	goto :FINISH
)

if /I "!COOK_MODE!"=="force" (
	call :CLEAN_COOKED_OUTPUTS
	if errorlevel 1 (
		set "EXIT_RC=1"
		goto :FINISH
	)
	set "SPARKLE_FORCE_COOK_TOOL_BUILD=1"
)

echo.
echo [LOG] Checking cook tools...
call "%~dp0Internal\Cook\CookTools.bat" PrepareProjectCook !CONFIG!
if errorlevel 1 (
	set "EXIT_RC=1"
	goto :FINISH
)

echo.
echo [LOG] Cooking assets for target '!TARGET_PROJECT!' using !CONFIG!...
if not defined SPARKLE_LOG_LEVEL set "SPARKLE_LOG_LEVEL=warn"
"!ASSET_COOKER_EXE!" cook-project "!TARGET_PROJECT!" "!CONFIG!" --root "!ROOT_DIR!"
set "EXIT_RC=!ERRORLEVEL!"
goto :FINISH

:USAGE
echo.
echo Usage: Scripts\CookAllAssets.bat [^<ProjectName^>^|ALL] [DebugEditor^|DebugGame^|DevelopmentEditor^|DevelopmentGame^|ShippingEditor^|ShippingGame]
echo.
echo Examples:
echo   Scripts\CookAllAssets.bat
echo   Scripts\CookAllAssets.bat ALL
echo   Scripts\CookAllAssets.bat Showcase
echo   Scripts\CookAllAssets.bat Showcase DevelopmentGame
echo.
echo This entrypoint prepares AssetCooker and forwards the full project cook request.
set "EXIT_RC=1"
goto :FINISH

:CHOOSE_COOK_MODE
if defined SPARKLE_COOK_MODE (
	set "COOK_MODE=!SPARKLE_COOK_MODE!"
) else if "%INTERACTIVE%"=="1" (
	echo.
	echo Select cook mode:
	echo   [1] Incremental - keep existing cooked outputs and caches
	echo   [2] Force recook - delete cooked outputs and rebuild cook tools
	set /P "COOK_MODE_SEL=Enter choice [1-2]: "
	if "!COOK_MODE_SEL!"=="2" (
		set "COOK_MODE=force"
	) else if "!COOK_MODE_SEL!"=="1" (
		set "COOK_MODE=incremental"
	) else (
		echo [ERROR] Invalid cook mode selection.
		exit /B 1
	)
) else (
	set "COOK_MODE=incremental"
)

if /I "!COOK_MODE!"=="force" (
	echo [LOG] Cook mode: force recook.
	exit /B 0
)
if /I "!COOK_MODE!"=="incremental" (
	echo [LOG] Cook mode: incremental.
	exit /B 0
)

echo [ERROR] Unsupported SPARKLE_COOK_MODE '!COOK_MODE!'. Use incremental or force.
exit /B 1

:CLEAN_COOKED_OUTPUTS
if /I "!TARGET_PROJECT!"=="ALL" (
	set "CLEAN_COOKED_PATH=!BUILD_DIR!\Cooked"
) else (
	set "CLEAN_COOKED_PATH=!BUILD_DIR!\Cooked\!TARGET_PROJECT!"
)

if exist "!CLEAN_COOKED_PATH!" (
	echo [LOG] Removing cooked outputs: !CLEAN_COOKED_PATH!
	rmdir /s /q "!CLEAN_COOKED_PATH!"
	if errorlevel 1 (
		echo [ERROR] Failed to remove cooked outputs: !CLEAN_COOKED_PATH!
		exit /B 1
	)
) else (
	echo [LOG] No cooked outputs to remove: !CLEAN_COOKED_PATH!
)

exit /B 0

:FINISH
set "_TMP_RC=%EXIT_RC%"
set "_TMP_INTERACTIVE=%INTERACTIVE%"
set "_TMP_TARGET_PROJECT=%TARGET_PROJECT%"
set "_TMP_CONFIG=%CONFIG%"
set "_TMP_COOK_MODE=%COOK_MODE%"
endlocal & set "EXIT_RC=%_TMP_RC%" & set "_INTERACTIVE=%_TMP_INTERACTIVE%" & set "_TARGET_PROJECT=%_TMP_TARGET_PROJECT%" & set "_CONFIG=%_TMP_CONFIG%" & set "_COOK_MODE=%_TMP_COOK_MODE%"

if "%_INTERACTIVE%"=="0" (
	set "_INTERACTIVE="
	set "_TARGET_PROJECT="
	set "_CONFIG="
	set "_COOK_MODE="
	exit /B %EXIT_RC%
)
set "_INTERACTIVE="

echo.
echo ============================================================
if "%EXIT_RC%"=="0" (
	echo   [SUCCESS] CookAllAssets completed successfully.
) else (
	echo   [ERROR] CookAllAssets completed with errors.
)
echo   Target: %_TARGET_PROJECT%
echo   Configuration: %_CONFIG%
if defined _COOK_MODE echo   Cook mode: %_COOK_MODE%
echo ============================================================
echo.
echo [LOG] Logs: %LOGFILE%
set "_TARGET_PROJECT="
set "_CONFIG="
set "_COOK_MODE="
pause
exit /B %EXIT_RC%
