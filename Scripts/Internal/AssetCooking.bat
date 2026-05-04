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
::   call "Internal\AssetCooking.bat" PrepareShaderCompiler <Configuration>
::   call "Internal\AssetCooking.bat" PrepareTextureCooker <Configuration>
::   call "Internal\AssetCooking.bat" PrepareAssetConverter <Configuration>
::
:: Outputs:
::   ASSET_CONVERTER_EXE - Absolute path to AssetConverter.exe
::   SHADER_COMPILER_EXE - Absolute path to ShaderCompiler.exe
::   TEXTURE_COOKER_EXE  - Absolute path to TextureCooker.exe
::   ASSET_COOKING_RC    - Return code from the preparation workflow
:: ============================================================================

setlocal enabledelayedexpansion

set "ASSET_CONVERTER_EXE="
set "SHADER_COMPILER_EXE="
set "TEXTURE_COOKER_EXE="

if /I "%~1"=="PrepareCookTools" (
    set "REQUEST_ASSET_CONVERTER=1"
    set "REQUEST_TEXTURE_COOKER=1"
    set "REQUEST_SHADER_COMPILER=1"
    set "BUILD_TARGETS=AssetConverter TextureCooker ShaderCompiler"
    set "BUILD_LABEL=AssetConverter, TextureCooker, and ShaderCompiler"
    goto :PREPARE_SELECTED_TOOLS
)

if /I "%~1"=="PrepareShaderCompiler" (
    set "REQUEST_SHADER_COMPILER=1"
    set "BUILD_TARGETS=ShaderCompiler"
    set "BUILD_LABEL=ShaderCompiler"
    goto :PREPARE_SELECTED_TOOLS
)

if /I "%~1"=="PrepareTextureCooker" (
    set "REQUEST_ASSET_CONVERTER=1"
    set "REQUEST_TEXTURE_COOKER=1"
    set "BUILD_TARGETS=AssetConverter TextureCooker"
    set "BUILD_LABEL=AssetConverter and TextureCooker"
    goto :PREPARE_SELECTED_TOOLS
)

if /I "%~1"=="PrepareAssetConverter" (
    set "REQUEST_ASSET_CONVERTER=1"
    set "BUILD_TARGETS=AssetConverter"
    set "BUILD_LABEL=AssetConverter"
    goto :PREPARE_SELECTED_TOOLS
)

echo [ERROR] AssetCooking.bat requires a valid command.
echo         Supported commands: PrepareCookTools, PrepareShaderCompiler, PrepareTextureCooker, PrepareAssetConverter
set "ASSET_COOKING_RC=1"
goto :FINISH

:PREPARE_SELECTED_TOOLS
call "%~dp0Config.bat"

set "CONFIG=%~2"
if "%CONFIG%"=="" set "CONFIG=Debug"

if "!REQUEST_SHADER_COMPILER!"=="1" (
    call :LOCATE_SHADER_COMPILER_EXE "!CONFIG!"
    if defined SHADER_COMPILER_EXE (
        call :IS_SHADER_COMPILER_STALE "!SHADER_COMPILER_EXE!"
        if "!ERRORLEVEL!"=="0" (
            echo.
            echo [LOG] Step 0/3: ShaderCompiler is up to date. Skipping GenerateSolution and build.
            set "ASSET_COOKING_RC=0"
            goto :FINISH
        )

        echo.
        echo [LOG] Step 0/3: ShaderCompiler inputs changed. Refreshing build files and rebuilding tool...
    ) else (
        echo.
        echo [LOG] Step 0/3: ShaderCompiler.exe missing. Preparing tool build...
    )
)

echo.
echo [LOG] Step 1/3: Validating build tools...
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\Internal\CheckToolchain.bat" CONTINUE
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
echo [LOG] Step 3/3: Building !BUILD_LABEL!...
for %%T in (!BUILD_TARGETS!) do (
    echo [LOG] Building target %%T...
    call "%~dp0CMakeHelpers.bat" BuildTargets !CONFIG! %%T
    set "BUILD_RC=!ERRORLEVEL!"
    if "!BUILD_RC!" NEQ "0" (
        echo [ERROR] Failed to build target %%T.
        set "ASSET_COOKING_RC=1"
        goto :FINISH
    )
)
if "!REQUEST_ASSET_CONVERTER!"=="1" (
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
)

if "!REQUEST_TEXTURE_COOKER!"=="1" (
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
)

if "!REQUEST_SHADER_COMPILER!"=="1" (
    call :LOCATE_SHADER_COMPILER_EXE "!CONFIG!"

    if not defined SHADER_COMPILER_EXE (
        echo [ERROR] ShaderCompiler.exe was not found after build.
        set "ASSET_COOKING_RC=1"
        goto :FINISH
    )
)

set "ASSET_COOKING_RC=0"
goto :FINISH

:LOCATE_SHADER_COMPILER_EXE
set "SHADER_COMPILER_EXE="
for %%P in (
    "!BUILD_DIR!\bin\%~1\ShaderCompiler.exe"
    "!BUILD_DIR!\bin\ShaderCompiler.exe"
    "!BIN_DIR!\%~1\ShaderCompiler.exe"
    "!BIN_DIR!\ShaderCompiler.exe"
) do (
    if not defined SHADER_COMPILER_EXE (
        if exist "%%~P" set "SHADER_COMPILER_EXE=%%~fP"
    )
)
exit /B 0

:IS_SHADER_COMPILER_STALE
set "_STALE_RC=1"
set "_POWERSHELL_EXE=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%_POWERSHELL_EXE%" set "_POWERSHELL_EXE=powershell"

"%_POWERSHELL_EXE%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0Test-ToolInputsNewerThan.ps1" ^
    -ReferencePath "%~1" ^
    -PathList "!ROOT_DIR!\Tools\ShaderCompiler;!ROOT_DIR!\Engine\RHI;!ROOT_DIR!\CMake;!ROOT_DIR!\CMakeLists.txt;!ROOT_DIR!\Engine\CMakeLists.txt;!ROOT_DIR!\Engine\RHI\CMakeLists.txt;!ROOT_DIR!\Tools\CMakeLists.txt;!ROOT_DIR!\Tools\ShaderCompiler\CMakeLists.txt"
set "_STALE_RC=%ERRORLEVEL%"
exit /B %_STALE_RC%

:FINISH
set "_TMP_ASSET_CONVERTER_EXE=%ASSET_CONVERTER_EXE%"
set "_TMP_SHADER_COMPILER_EXE=%SHADER_COMPILER_EXE%"
set "_TMP_TEXTURE_COOKER_EXE=%TEXTURE_COOKER_EXE%"
set "_TMP_RC=%ASSET_COOKING_RC%"
endlocal & set "ASSET_CONVERTER_EXE=%_TMP_ASSET_CONVERTER_EXE%" & set "SHADER_COMPILER_EXE=%_TMP_SHADER_COMPILER_EXE%" & set "TEXTURE_COOKER_EXE=%_TMP_TEXTURE_COOKER_EXE%" & set "ASSET_COOKING_RC=%_TMP_RC%" & exit /B %_TMP_RC%