@echo off
:: ============================================================================
:: CookAllAssets.bat - Preferred top-level full asset cook entrypoint
:: ============================================================================
:: Launch shim for the AssetCooker project planner.
::
:: Usage:
::   CookAllAssets.bat [ProjectName|ALL] [Debug|Release|RelWithDebInfo]
:: ============================================================================

setlocal enabledelayedexpansion

set "INTERACTIVE=1"
if defined PARENT_BATCH set "INTERACTIVE=0"

call "%~dp0Internal\Config.bat"

set "EXIT_RC=1"
set "TARGET_PROJECT=%~1"
set "CONFIG=%~2"
if defined TARGET_PROJECT set "TARGET_PROJECT=!TARGET_PROJECT: =!"
if defined CONFIG set "CONFIG=!CONFIG: =!"

if /I "%TARGET_PROJECT%"=="/h" goto :USAGE
if /I "%TARGET_PROJECT%"=="-h" goto :USAGE
if /I "%TARGET_PROJECT%"=="/help" goto :USAGE
if /I "%TARGET_PROJECT%"=="--help" goto :USAGE

if not defined TARGET_PROJECT set "TARGET_PROJECT=ALL"
if "%CONFIG%"=="" set "CONFIG=Debug"

echo [LOG] Project target: !TARGET_PROJECT!
echo [LOG] Configuration: !CONFIG!

call "%~dp0Internal\AssetCooking.bat" PrepareAssetCooker !CONFIG!
if errorlevel 1 (
	set "EXIT_RC=1"
	goto :FINISH
)

"!ASSET_COOKER_EXE!" cook-project "!TARGET_PROJECT!" "!CONFIG!" --root "!ROOT_DIR!"
set "EXIT_RC=!ERRORLEVEL!"
goto :FINISH

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
echo This shim prepares AssetCooker and forwards the full project cook request.
set "EXIT_RC=1"

:FINISH
set "_TMP_RC=%EXIT_RC%"
set "_TMP_INTERACTIVE=%INTERACTIVE%"
set "_TMP_TARGET_PROJECT=%TARGET_PROJECT%"
set "_TMP_CONFIG=%CONFIG%"
endlocal & set "EXIT_RC=%_TMP_RC%" & set "_INTERACTIVE=%_TMP_INTERACTIVE%" & set "_TARGET_PROJECT=%_TMP_TARGET_PROJECT%" & set "_CONFIG=%_TMP_CONFIG%"

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
	echo   [SUCCESS] CookAllAssets completed successfully.
) else (
	echo   [ERROR] CookAllAssets completed with errors.
)
echo   Target: %_TARGET_PROJECT%
echo   Configuration: %_CONFIG%
echo ============================================================
echo.
set "_TARGET_PROJECT="
set "_CONFIG="
pause
exit /B %EXIT_RC%