@echo off
:: ============================================================================
:: CookTools.bat - Shared cook-tool preparation helpers
:: ============================================================================
:: Internal helper module that keeps the asset and shader cooking entrypoints
:: on one preflight path for toolchain validation, configure/sync, cook-tool
:: build, and executable discovery.
::
:: Usage:
::   call "Internal\Cook\CookTools.bat" PrepareCookTools <Configuration>
::   call "Internal\Cook\CookTools.bat" PrepareAssetCooker <Configuration>
::   call "Internal\Cook\CookTools.bat" PrepareShaderCompiler <Configuration>
::   call "Internal\Cook\CookTools.bat" PrepareTextureCooker <Configuration>
::
:: Outputs:
::   ASSET_COOKER_EXE    - Absolute path to AssetCooker.exe
::   SHADER_COMPILER_EXE - Absolute path to ShaderCompiler.exe
::   TEXTURE_COOKER_EXE  - Absolute path to TextureCooker.exe
::   ASSET_COOKING_RC    - Return code from the preparation workflow
:: ============================================================================

setlocal enabledelayedexpansion

set "ASSET_COOKER_EXE="
set "SHADER_COMPILER_EXE="
set "TEXTURE_COOKER_EXE="

if /I "%~1"=="PrepareCookTools" (
    set "REQUEST_ASSET_COOKER=1"
    set "REQUEST_TEXTURE_COOKER=1"
    set "REQUEST_SHADER_COMPILER=1"
    set "BUILD_TARGETS=SparkleCookTools"
    set "BUILD_LABEL=AssetCooker, TextureCooker, and ShaderCompiler"
    goto :PREPARE_SELECTED_TOOLS
)

if /I "%~1"=="PrepareAssetCooker" (
    set "REQUEST_ASSET_COOKER=1"
    set "REQUEST_TEXTURE_COOKER=1"
    set "REQUEST_SHADER_COMPILER=1"
    set "BUILD_TARGETS=SparkleCookTools"
    set "BUILD_LABEL=AssetCooker, TextureCooker, and ShaderCompiler"
    goto :PREPARE_SELECTED_TOOLS
)

if /I "%~1"=="PrepareShaderCompiler" (
    set "REQUEST_SHADER_COMPILER=1"
    set "BUILD_TARGETS=ShaderCompiler"
    set "BUILD_LABEL=ShaderCompiler"
    goto :PREPARE_SELECTED_TOOLS
)

if /I "%~1"=="PrepareTextureCooker" (
    set "REQUEST_TEXTURE_COOKER=1"
    set "BUILD_TARGETS=TextureCooker"
    set "BUILD_LABEL=TextureCooker"
    goto :PREPARE_SELECTED_TOOLS
)

echo [ERROR] CookTools.bat requires a valid command.
echo         Supported commands: PrepareCookTools, PrepareAssetCooker, PrepareShaderCompiler, PrepareTextureCooker
set "ASSET_COOKING_RC=1"
goto :FINISH

:PREPARE_SELECTED_TOOLS
call "%~dp0..\Core\Config.bat"

set "CONFIG=%~2"
if "%CONFIG%"=="" set "CONFIG=Debug"

if "!REQUEST_SHADER_COMPILER!"=="1" if "!REQUEST_ASSET_COOKER!" NEQ "1" if "!REQUEST_TEXTURE_COOKER!" NEQ "1" (
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
call "!SCRIPTS_DIR!\Internal\Toolchain\CheckToolchain.bat" CONTINUE
set "TOOLCHAIN_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!TOOLCHAIN_RC!" NEQ "0" (
    echo [ERROR] Toolchain validation failed.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

echo.
echo [LOG] Step 2/3: Ensuring build files are current...
call "!SCRIPTS_DIR!\Internal\Build\EnsureBuildFiles.bat"
set "ENSURE_RC=!ERRORLEVEL!"
if "!ENSURE_RC!" NEQ "0" (
    echo [ERROR] Build-file preparation failed.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

echo.
echo [LOG] Step 3/3: Building !BUILD_LABEL!...
echo [LOG] Building targets !BUILD_TARGETS!...
call "%~dp0..\Build\CMakeHelpers.bat" BuildTargets !CONFIG! !BUILD_TARGETS!
set "BUILD_RC=!ERRORLEVEL!"
if "!BUILD_RC!" NEQ "0" (
    echo [ERROR] Failed to build targets !BUILD_TARGETS!.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)
if "!REQUEST_ASSET_COOKER!"=="1" (
    for %%P in (
        "!BUILD_DIR!\bin\!CONFIG!\AssetCooker.exe"
        "!BUILD_DIR!\bin\AssetCooker.exe"
        "!BIN_DIR!\!CONFIG!\AssetCooker.exe"
        "!BIN_DIR!\AssetCooker.exe"
    ) do (
        if not defined ASSET_COOKER_EXE (
            if exist "%%~P" set "ASSET_COOKER_EXE=%%~fP"
        )
    )

    if not defined ASSET_COOKER_EXE (
        echo [ERROR] AssetCooker.exe was not found after build.
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

"%_POWERSHELL_EXE%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\Build\Test-ToolInputsNewerThan.ps1" ^
    -ReferencePath "%~1" ^
    -PathList "!ROOT_DIR!\Tools\ShaderCompiler;!ROOT_DIR!\Engine\RHI;!ROOT_DIR!\CMake;!ROOT_DIR!\CMakeLists.txt;!ROOT_DIR!\Engine\CMakeLists.txt;!ROOT_DIR!\Engine\RHI\CMakeLists.txt;!ROOT_DIR!\Tools\CMakeLists.txt;!ROOT_DIR!\Tools\ShaderCompiler\CMakeLists.txt"
set "_STALE_RC=%ERRORLEVEL%"
exit /B %_STALE_RC%

:FINISH
set "_TMP_ASSET_COOKER_EXE=%ASSET_COOKER_EXE%"
set "_TMP_SHADER_COMPILER_EXE=%SHADER_COMPILER_EXE%"
set "_TMP_TEXTURE_COOKER_EXE=%TEXTURE_COOKER_EXE%"
set "_TMP_RC=%ASSET_COOKING_RC%"
endlocal & set "ASSET_COOKER_EXE=%_TMP_ASSET_COOKER_EXE%" & set "SHADER_COMPILER_EXE=%_TMP_SHADER_COMPILER_EXE%" & set "TEXTURE_COOKER_EXE=%_TMP_TEXTURE_COOKER_EXE%" & set "ASSET_COOKING_RC=%_TMP_RC%" & exit /B %_TMP_RC%
