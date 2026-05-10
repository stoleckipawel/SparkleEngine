@echo off
:: ============================================================================
:: CookTextures.bat - Cook texture assets for a project
:: ============================================================================
:: Launch entrypoint for the AssetCooker texture plan step.
:: ============================================================================

setlocal enabledelayedexpansion

set "INTERACTIVE=1"
if defined PARENT_BATCH set "INTERACTIVE=0"

if not defined LOG_CAPTURED (
	call "%~dp0..\Internal\Core\BootstrapLog.bat" "%~f0" %*
	set "BOOTSTRAP_RC=!ERRORLEVEL!"
	exit /B !BOOTSTRAP_RC!
)

call "%~dp0..\Internal\Core\Config.bat"

set "EXIT_RC=1"
set "TARGET_PROJECT=%~1"
set "CONFIG=%~2"
if defined TARGET_PROJECT set "TARGET_PROJECT=!TARGET_PROJECT: =!"
if defined CONFIG set "CONFIG=!CONFIG: =!"

if /I "%TARGET_PROJECT%"=="/h" goto :USAGE
if /I "%TARGET_PROJECT%"=="-h" goto :USAGE
if /I "%TARGET_PROJECT%"=="/help" goto :USAGE
if /I "%TARGET_PROJECT%"=="--help" goto :USAGE
if not defined TARGET_PROJECT goto :USAGE
if "%CONFIG%"=="" set "CONFIG=DevelopmentGame"

echo [LOG] Project: !TARGET_PROJECT!
echo [LOG] Configuration: !CONFIG!

echo [LOG] Checking cook tools...
call "%~dp0..\Internal\Cook\CookTools.bat" PrepareTextureCook !CONFIG!
if errorlevel 1 goto :FINISH

if not defined SPARKLE_LOG_LEVEL set "SPARKLE_LOG_LEVEL=warn"
"!ASSET_COOKER_EXE!" cook-textures "!TARGET_PROJECT!" "!CONFIG!" --root "!ROOT_DIR!"
set "EXIT_RC=!ERRORLEVEL!"
goto :FINISH

:USAGE
echo.
echo Usage: Scripts\Cook\CookTextures.bat ^<ProjectName^> [DebugEditor^|DebugGame^|DevelopmentEditor^|DevelopmentGame^|ShippingEditor^|ShippingGame]
echo.
echo Examples:
echo   Scripts\Cook\CookTextures.bat Showcase
echo   Scripts\Cook\CookTextures.bat Showcase DevelopmentGame
echo.
echo This entrypoint forwards texture cooking to AssetCooker.
set "EXIT_RC=1"

:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
set "_TMP_INTERACTIVE=%INTERACTIVE%"
set "_TMP_TARGET_PROJECT=%TARGET_PROJECT%"
set "_TMP_CONFIG=%CONFIG%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%" & set "_INTERACTIVE=%_TMP_INTERACTIVE%" & set "_TARGET_PROJECT=%_TMP_TARGET_PROJECT%" & set "_CONFIG=%_TMP_CONFIG%"

if "%_INTERACTIVE%"=="0" (
	set "_INTERACTIVE="
	set "_TARGET_PROJECT="
	set "_CONFIG="
	exit /B %EXIT_RC%
)
set "_INTERACTIVE="

echo.
echo ============================================================
if "%EXIT_RC%"=="0" (
	echo   [SUCCESS] CookTextures completed successfully.
) else (
	echo   [ERROR] CookTextures completed with errors.
)
echo   Project: %_TARGET_PROJECT%
echo   Configuration: %_CONFIG%
echo ============================================================
echo.
echo [LOG] Logs: %LOGFILE%
set "_TARGET_PROJECT="
set "_CONFIG="
pause
exit /B %EXIT_RC%
