@echo off
:: ============================================================================
:: AssetCooking.bat - Shared cook-tool preparation helpers
:: ============================================================================
:: Internal helper module that keeps the asset and shader cooking entrypoints
:: on one preflight path for toolchain validation, configure/sync, cook-tool
:: build, and executable discovery.
::
:: Usage:
::   call "Internal\AssetCooking.bat" PrepareCookTools <Configuration>
::
:: Outputs:
::   ASSET_CONVERTER_EXE - Absolute path to AssetConverter.exe
::   SHADER_COMPILER_EXE - Absolute path to ShaderCompiler.exe
::   TEXTURE_COOKER_EXE  - Absolute path to TextureCooker.exe
::   ASSET_COOKING_RC    - Return code from the preparation workflow
:: ============================================================================

setlocal enabledelayedexpansion

if /I "%~1"=="PrepareCookTools" goto :PREPARE_COOK_TOOLS
if /I "%~1"=="PrepareAssetConverter" goto :PREPARE_COOK_TOOLS

echo [ERROR] AssetCooking.bat requires a valid command.
echo         Supported commands: PrepareCookTools
set "ASSET_COOKING_RC=1"
goto :FINISH

:PREPARE_COOK_TOOLS
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
echo [LOG] Step 3/3: Building AssetConverter, TextureCooker, and ShaderCompiler...
call "%~dp0CMakeHelpers.bat" BuildTargets !CONFIG! AssetConverter TextureCooker ShaderCompiler
set "BUILD_RC=!ERRORLEVEL!"
if "!BUILD_RC!" NEQ "0" (
    echo [ERROR] Failed to build one or more cook-tool targets.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

set "ASSET_CONVERTER_EXE="
set "SHADER_COMPILER_EXE="
set "TEXTURE_COOKER_EXE="
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

for %%P in (
    "!BUILD_DIR!\bin\!CONFIG!\TextureCooker.exe"
    "!BUILD_DIR!\bin\TextureCooker.exe"
    "!BIN_DIR!\!CONFIG!\TextureCooker.exe"
    "!BIN_DIR!\TextureCooker.exe"
) do (
    if not defined TEXTURE_COOKER_EXE (
        if exist "%%~P" set "TEXTURE_COOKER_EXE=%%~fP"
    )
)

if not defined TEXTURE_COOKER_EXE (
    echo [ERROR] TextureCooker.exe was not found after build.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

for %%P in (
    "!BUILD_DIR!\bin\!CONFIG!\ShaderCompiler.exe"
    "!BUILD_DIR!\bin\ShaderCompiler.exe"
    "!BIN_DIR!\!CONFIG!\ShaderCompiler.exe"
    "!BIN_DIR!\ShaderCompiler.exe"
) do (
    if not defined SHADER_COMPILER_EXE (
        if exist "%%~P" set "SHADER_COMPILER_EXE=%%~fP"
    )
)

if not defined SHADER_COMPILER_EXE (
    echo [ERROR] ShaderCompiler.exe was not found after build.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

set "ASSET_COOKING_RC=0"
goto :FINISH

:FINISH
set "_TMP_ASSET_CONVERTER_EXE=%ASSET_CONVERTER_EXE%"
set "_TMP_SHADER_COMPILER_EXE=%SHADER_COMPILER_EXE%"
set "_TMP_TEXTURE_COOKER_EXE=%TEXTURE_COOKER_EXE%"
set "_TMP_RC=%ASSET_COOKING_RC%"
endlocal & set "ASSET_CONVERTER_EXE=%_TMP_ASSET_CONVERTER_EXE%" & set "SHADER_COMPILER_EXE=%_TMP_SHADER_COMPILER_EXE%" & set "TEXTURE_COOKER_EXE=%_TMP_TEXTURE_COOKER_EXE%" & set "ASSET_COOKING_RC=%_TMP_RC%" & exit /B %_TMP_RC%