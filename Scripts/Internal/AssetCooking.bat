@echo off
:: ============================================================================
:: AssetCooking.bat - Shared AssetConverter preparation helpers
:: ============================================================================
:: Internal helper module that keeps the scene-cooking entrypoints on one
:: preflight path for toolchain validation, configure/sync, AssetConverter
:: build, and executable discovery.
::
:: Usage:
::   call "Internal\AssetCooking.bat" PrepareAssetConverter <Configuration>
::
:: Outputs:
::   ASSET_CONVERTER_EXE - Absolute path to AssetConverter.exe
::   ASSET_COOKING_RC    - Return code from the preparation workflow
:: ============================================================================

setlocal enabledelayedexpansion

if /I "%~1"=="PrepareAssetConverter" goto :PREPARE_ASSET_CONVERTER

echo [ERROR] AssetCooking.bat requires a valid command.
echo         Supported commands: PrepareAssetConverter
set "ASSET_COOKING_RC=1"
goto :FINISH

:PREPARE_ASSET_CONVERTER
call "%~dp0Config.bat"

set "CONFIG=%~2"
if "%CONFIG%"=="" set "CONFIG=Debug"

echo.
echo [LOG] Step 1/3: Validating build tools...
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\CheckToolchain.bat" CONTINUE
set "TOOLCHAIN_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!TOOLCHAIN_RC!" NEQ "0" (
    echo [ERROR] Toolchain validation failed.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

echo.
echo [LOG] Step 2/3: Refreshing build files and syncing third-party dependencies...
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\GenerateSolution.bat" CONTINUE
set "CONFIGURE_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!CONFIGURE_RC!" NEQ "0" (
    echo [ERROR] GenerateSolution step failed.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

echo.
echo [LOG] Step 3/3: Building AssetConverter...
call "%~dp0CMakeHelpers.bat" BuildTargets !CONFIG! AssetConverter
set "BUILD_RC=!ERRORLEVEL!"
if "!BUILD_RC!" NEQ "0" (
    echo [ERROR] Failed to build the AssetConverter target.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

set "ASSET_CONVERTER_EXE="
for %%P in (
    "!BUILD_DIR!\bin\!CONFIG!\AssetConverter.exe"
    "!BUILD_DIR!\bin\AssetConverter.exe"
    "!BIN_DIR!\!CONFIG!\AssetConverter.exe"
    "!BIN_DIR!\AssetConverter.exe"
) do (
    if not defined ASSET_CONVERTER_EXE (
        if exist "%%~P" set "ASSET_CONVERTER_EXE=%%~fP"
    )
)

if not defined ASSET_CONVERTER_EXE (
    echo [ERROR] AssetConverter.exe was not found after build.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

set "ASSET_COOKING_RC=0"
goto :FINISH

:FINISH
set "_TMP_EXE=%ASSET_CONVERTER_EXE%"
set "_TMP_RC=%ASSET_COOKING_RC%"
endlocal & set "ASSET_CONVERTER_EXE=%_TMP_EXE%" & set "ASSET_COOKING_RC=%_TMP_RC%" & exit /B %_TMP_RC%