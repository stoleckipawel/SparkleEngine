@echo off
:: ============================================================================
:: CookTools.bat - Shared cook-tool preparation helpers
:: ============================================================================
:: Internal helper module that keeps the asset and shader cooking entrypoints
:: on one preflight path for executable discovery and missing-tool builds.
:: Requested tools that are already present are used as-is. Requested tools
:: that are missing are built before cooking.
:: Cook output configurations may be editor or game profiles, but cook tools are
:: always built and discovered with the matching editor profile.
::
:: Usage:
::   call "Internal\Cook\CookTools.bat" PrepareCookTools <Configuration>
::   call "Internal\Cook\CookTools.bat" PrepareProjectCook <Configuration>
::   call "Internal\Cook\CookTools.bat" PrepareSceneCook <Configuration>
::   call "Internal\Cook\CookTools.bat" PrepareTextureCook <Configuration>
::   call "Internal\Cook\CookTools.bat" PrepareShaderCook <Configuration>
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
set "BUILD_TARGETS="
set "BUILD_LABEL="
set "REQUESTED_COOK_TOOLS="
set "TOOL_CONFIG="
set "SUPPORTED_COOK_CONFIGS=DebugEditor DebugGame DevelopmentEditor DevelopmentGame ShippingEditor ShippingGame"
set "SUPPORTED_TOOL_CONFIGS=DebugEditor DevelopmentEditor ShippingEditor"

if /I "%~1"=="PrepareCookTools" goto :REQUEST_PROJECT_COOK
if /I "%~1"=="PrepareProjectCook" goto :REQUEST_PROJECT_COOK
if /I "%~1"=="PrepareSceneCook" goto :REQUEST_SCENE_COOK
if /I "%~1"=="PrepareTextureCook" goto :REQUEST_TEXTURE_COOK
if /I "%~1"=="PrepareShaderCook" goto :REQUEST_SHADER_COOK
if /I "%~1"=="PrepareAssetCooker" goto :REQUEST_ASSET_COOKER
if /I "%~1"=="PrepareTextureCooker" goto :REQUEST_TEXTURE_COOKER
if /I "%~1"=="PrepareShaderCompiler" goto :REQUEST_SHADER_COMPILER

echo [ERROR] CookTools.bat requires a valid command.
echo         Supported commands: PrepareCookTools, PrepareProjectCook, PrepareSceneCook, PrepareTextureCook, PrepareShaderCook, PrepareAssetCooker, PrepareShaderCompiler, PrepareTextureCooker
set "ASSET_COOKING_RC=1"
goto :FINISH

:REQUEST_PROJECT_COOK
set "REQUESTED_COOK_TOOLS=AssetCooker TextureCooker ShaderCompiler"
goto :PREPARE_SELECTED_TOOLS

:REQUEST_SCENE_COOK
set "REQUESTED_COOK_TOOLS=AssetCooker"
goto :PREPARE_SELECTED_TOOLS

:REQUEST_TEXTURE_COOK
set "REQUESTED_COOK_TOOLS=AssetCooker TextureCooker"
goto :PREPARE_SELECTED_TOOLS

:REQUEST_SHADER_COOK
set "REQUESTED_COOK_TOOLS=AssetCooker ShaderCompiler"
goto :PREPARE_SELECTED_TOOLS

:REQUEST_ASSET_COOKER
set "REQUESTED_COOK_TOOLS=AssetCooker"
goto :PREPARE_SELECTED_TOOLS

:REQUEST_TEXTURE_COOKER
set "REQUESTED_COOK_TOOLS=TextureCooker"
goto :PREPARE_SELECTED_TOOLS

:REQUEST_SHADER_COMPILER
set "REQUESTED_COOK_TOOLS=ShaderCompiler"
goto :PREPARE_SELECTED_TOOLS

:PREPARE_SELECTED_TOOLS
call "%~dp0..\Core\Config.bat"

set "CONFIG=%~2"
if "%CONFIG%"=="" set "CONFIG=DevelopmentEditor"

call :RESOLVE_TOOL_CONFIG "!CONFIG!"
if "!ERRORLEVEL!" NEQ "0" (
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

if /I not "!CONFIG!"=="!TOOL_CONFIG!" (
    echo [LOG] Cook tool configuration: !TOOL_CONFIG! for cook configuration !CONFIG!.
)

call :LOCATE_REQUESTED_TOOLS "!TOOL_CONFIG!"

if /I "!SPARKLE_FORCE_COOK_TOOL_BUILD!"=="1" (
    set "BUILD_TARGETS="
    set "BUILD_LABEL="
    for %%T in (!REQUESTED_COOK_TOOLS!) do call :ADD_BUILD_TARGET %%T %%T
    echo [LOG] SPARKLE_FORCE_COOK_TOOL_BUILD=1. Rebuilding requested cook tools.
)

if /I not "!SPARKLE_FORCE_COOK_TOOL_BUILD!"=="1" call :PLAN_MISSING_REQUESTED_TOOL_BUILD

if not defined BUILD_TARGETS (
    echo [LOG] Required cook tools are present. Skipping toolchain validation, GenerateSolution, and build.
    set "ASSET_COOKING_RC=0"
    goto :FINISH
)

goto :BUILD_REQUESTED_TOOLS

:BUILD_REQUESTED_TOOLS
if /I "!SPARKLE_FORCE_COOK_TOOL_BUILD!"=="1" (
    echo [LOG] Preparing requested cook tools: !BUILD_LABEL!.
) else (
    echo [LOG] Missing cook tools detected. Building required targets: !BUILD_LABEL!.
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
echo [LOG] Step 3/3: Building !BUILD_LABEL! with !TOOL_CONFIG!...
echo [LOG] Building targets !BUILD_TARGETS!...
call "%~dp0..\Build\CMakeHelpers.bat" BuildTargets !TOOL_CONFIG! !BUILD_TARGETS!
set "BUILD_RC=!ERRORLEVEL!"
if "!BUILD_RC!" NEQ "0" (
    echo [ERROR] Failed to build targets !BUILD_TARGETS!.
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)
call :LOCATE_REQUESTED_TOOLS "!TOOL_CONFIG!"
call :VALIDATE_REQUESTED_TOOLS_BUILT
if "!ERRORLEVEL!" NEQ "0" (
    set "ASSET_COOKING_RC=1"
    goto :FINISH
)

set "ASSET_COOKING_RC=0"
goto :FINISH

:RESOLVE_TOOL_CONFIG
set "TOOL_CONFIG=%~1"
if /I "!TOOL_CONFIG:~-4!"=="Game" set "TOOL_CONFIG=!TOOL_CONFIG:~0,-4!Editor"

call :IS_SUPPORTED_TOOL_CONFIG "!TOOL_CONFIG!"
if "!ERRORLEVEL!" NEQ "0" (
    echo [ERROR] Unsupported cook configuration '%~1'.
    echo         Use !SUPPORTED_COOK_CONFIGS!.
    exit /B 1
)
exit /B 0

:IS_SUPPORTED_TOOL_CONFIG
for %%C in (!SUPPORTED_TOOL_CONFIGS!) do (
    if /I "%~1"=="%%C" exit /B 0
)
exit /B 1

:LOCATE_REQUESTED_TOOLS
for %%T in (!REQUESTED_COOK_TOOLS!) do (
    if /I "%%T"=="AssetCooker" call :LOCATE_ASSET_COOKER_EXE "%~1"
    if /I "%%T"=="TextureCooker" call :LOCATE_TEXTURE_COOKER_EXE "%~1"
    if /I "%%T"=="ShaderCompiler" call :LOCATE_SHADER_COMPILER_EXE "%~1"
)
exit /B 0

:VALIDATE_REQUESTED_TOOLS_BUILT
set "_MISSING_LABEL="
for %%T in (!REQUESTED_COOK_TOOLS!) do (
    set "_TOOL_PRESENT=0"
    if /I "%%T"=="AssetCooker" if defined ASSET_COOKER_EXE set "_TOOL_PRESENT=1"
    if /I "%%T"=="TextureCooker" if defined TEXTURE_COOKER_EXE set "_TOOL_PRESENT=1"
    if /I "%%T"=="ShaderCompiler" if defined SHADER_COMPILER_EXE set "_TOOL_PRESENT=1"
    if "!_TOOL_PRESENT!" NEQ "1" (
        if defined _MISSING_LABEL (set "_MISSING_LABEL=!_MISSING_LABEL!, %%T") else (set "_MISSING_LABEL=%%T")
    )
)

if defined _MISSING_LABEL (
    echo [ERROR] Cook tools were not found after build: !_MISSING_LABEL!.
    exit /B 1
)
exit /B 0

:PLAN_MISSING_REQUESTED_TOOL_BUILD
for %%T in (!REQUESTED_COOK_TOOLS!) do (
    if /I "%%T"=="AssetCooker" if not defined ASSET_COOKER_EXE (
        echo [LOG] AssetCooker.exe missing. Tool build required.
        call :ADD_BUILD_TARGET AssetCooker AssetCooker
    )
    if /I "%%T"=="TextureCooker" if not defined TEXTURE_COOKER_EXE (
        echo [LOG] TextureCooker.exe missing. Tool build required.
        call :ADD_BUILD_TARGET TextureCooker TextureCooker
    )
    if /I "%%T"=="ShaderCompiler" if not defined SHADER_COMPILER_EXE (
        echo [LOG] ShaderCompiler.exe missing. Tool build required.
        call :ADD_BUILD_TARGET ShaderCompiler ShaderCompiler
    )
)
exit /B 0

:ADD_BUILD_TARGET
if not defined BUILD_TARGETS (
    set "BUILD_TARGETS=%~1"
) else (
    set "BUILD_TARGETS=!BUILD_TARGETS! %~1"
)

if not defined BUILD_LABEL (
    set "BUILD_LABEL=%~2"
) else (
    set "BUILD_LABEL=!BUILD_LABEL!, %~2"
)
exit /B 0

:LOCATE_ASSET_COOKER_EXE
set "ASSET_COOKER_EXE="
for %%P in (
    "!BUILD_DIR!\bin\%~1\AssetCooker.exe"
    "!BUILD_DIR!\bin\AssetCooker.exe"
    "!BIN_DIR!\%~1\AssetCooker.exe"
    "!BIN_DIR!\AssetCooker.exe"
) do (
    if not defined ASSET_COOKER_EXE (
        if exist "%%~P" set "ASSET_COOKER_EXE=%%~fP"
    )
)
exit /B 0

:LOCATE_TEXTURE_COOKER_EXE
set "TEXTURE_COOKER_EXE="
for %%P in (
    "!BUILD_DIR!\bin\%~1\TextureCooker.exe"
    "!BUILD_DIR!\bin\TextureCooker.exe"
    "!BIN_DIR!\%~1\TextureCooker.exe"
    "!BIN_DIR!\TextureCooker.exe"
) do (
    if not defined TEXTURE_COOKER_EXE (
        if exist "%%~P" set "TEXTURE_COOKER_EXE=%%~fP"
    )
)
exit /B 0

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

:FINISH
set "_TMP_ASSET_COOKER_EXE=%ASSET_COOKER_EXE%"
set "_TMP_SHADER_COMPILER_EXE=%SHADER_COMPILER_EXE%"
set "_TMP_TEXTURE_COOKER_EXE=%TEXTURE_COOKER_EXE%"
set "_TMP_RC=%ASSET_COOKING_RC%"
endlocal & set "ASSET_COOKER_EXE=%_TMP_ASSET_COOKER_EXE%" & set "SHADER_COMPILER_EXE=%_TMP_SHADER_COMPILER_EXE%" & set "TEXTURE_COOKER_EXE=%_TMP_TEXTURE_COOKER_EXE%" & set "ASSET_COOKING_RC=%_TMP_RC%" & exit /B %_TMP_RC%
