@echo off
:: ============================================================================
:: CookScene.bat - Build and run the AssetConverter for one project scene
:: ============================================================================
:: Makes scene cooking a normal contributor workflow:
::   1. Validates required tools and third-party dependencies
::   2. Generates the solution if needed
::   3. Builds the AssetConverter target for the requested configuration
::   4. Runs the converter from the selected project root
::
:: Usage:
::   CookScene.bat <ProjectName> <ScenePath> [Configuration]
::
:: Arguments:
::   ProjectName    - Project directory under Projects\, for example Showcase
::   ScenePath      - Path relative to Projects\<ProjectName>\Assets\Meshes\
::                    for example Sponza\Sponza.gltf
::   Configuration  - Debug | Release | RelWithDebInfo (default: Debug)
::
:: Examples:
::   CookScene.bat Showcase Sponza\Sponza.gltf
::   CookScene.bat Showcase Bistro\BistroExterior.fbx Release
::
:: Output locations:
::   Projects\<ProjectName>\Assets\Cooked\SceneManifests\
::   Projects\<ProjectName>\Assets\Cooked\Meshes\
::   Projects\<ProjectName>\Assets\Cooked\Materials\
::   Projects\<ProjectName>\Assets\Cooked\Textures\
:: ============================================================================

setlocal enabledelayedexpansion

if not defined LOG_CAPTURED (
    call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
    exit /B %ERRORLEVEL%
)

call "%~dp0Internal\Config.bat"

set "EXIT_RC=1"
set "TARGET_PROJECT=%~1"
set "SCENE_PATH=%~2"
set "CONFIG=%~3"

if /I "%TARGET_PROJECT%"=="/h" goto :USAGE
if /I "%TARGET_PROJECT%"=="-h" goto :USAGE
if /I "%TARGET_PROJECT%"=="/help" goto :USAGE
if /I "%TARGET_PROJECT%"=="--help" goto :USAGE

if "%TARGET_PROJECT%"=="" goto :MISSING_ARGS
if "%SCENE_PATH%"=="" goto :MISSING_ARGS
if "%CONFIG%"=="" set "CONFIG=Debug"

set "VALID_CONFIG=0"
for %%C in (Debug Release RelWithDebInfo) do (
    if /I "!CONFIG!"=="%%C" set "VALID_CONFIG=1"
)

if "!VALID_CONFIG!" NEQ "1" (
    echo [ERROR] Unsupported configuration '!CONFIG!'.
    goto :USAGE
)

set "PROJECT_ROOT=!PROJECTS_DIR!\!TARGET_PROJECT!"
if not exist "!PROJECT_ROOT!\.sparkle-project" (
    echo [ERROR] Project '!TARGET_PROJECT!' was not found under Projects\.
    echo         Expected marker: !PROJECT_ROOT!\.sparkle-project
    set "EXIT_RC=1"
    goto :FINISH
)

set "SCENE_SOURCE_PATH=!PROJECT_ROOT!\Assets\Meshes\!SCENE_PATH!"
if not exist "!SCENE_SOURCE_PATH!" (
    echo [ERROR] Scene source was not found.
    echo         Expected: !SCENE_SOURCE_PATH!
    set "EXIT_RC=1"
    goto :FINISH
)

echo [LOG] Project: !TARGET_PROJECT!
echo [LOG] Scene: !SCENE_PATH!
echo [LOG] Configuration: !CONFIG!

echo.
echo [LOG] Step 1/4: Validating build tools...
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\CheckDependencies.bat" CONTINUE
set "DEP_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!DEP_RC!" NEQ "0" (
    echo [ERROR] Dependency check failed.
    set "EXIT_RC=1"
    goto :FINISH
)

echo.
echo [LOG] Step 2/4: Checking third-party dependencies...
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\CheckThirdParty.bat"
set "TP_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!TP_RC!" NEQ "0" (
    echo [ERROR] Third-party dependency check failed.
    set "EXIT_RC=1"
    goto :FINISH
)

echo.
echo [LOG] Step 3/4: Refreshing the build files...
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\GenerateProjectFiles.bat" CONTINUE
set "GEN_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!GEN_RC!" NEQ "0" (
    echo [ERROR] Solution generation failed.
    set "EXIT_RC=1"
    goto :FINISH
)

echo [LOG] Using solution: !SOLUTION_FILE!

echo.
echo [LOG] Step 4/4: Building AssetConverter and cooking the scene...
cmake --build "!BUILD_DIR!" --config !CONFIG! --target AssetConverter -- /nologo /v:minimal
if errorlevel 1 (
    echo [ERROR] Failed to build the AssetConverter target.
    set "EXIT_RC=1"
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
    set "EXIT_RC=1"
    goto :FINISH
)

pushd "!PROJECT_ROOT!"
"!ASSET_CONVERTER_EXE!" "!SCENE_SOURCE_PATH!"
set "COOK_RC=!ERRORLEVEL!"
popd

if "!COOK_RC!" NEQ "0" (
    echo [ERROR] Scene cooking failed for '!SCENE_PATH!'.
    set "EXIT_RC=!COOK_RC!"
    goto :FINISH
)

echo.
echo [SUCCESS] Scene cooking completed.
echo [LOG] Source scene root:  Projects\!TARGET_PROJECT!\Assets\Meshes\
echo [LOG] Cooked output root: Projects\!TARGET_PROJECT!\Assets\Cooked\
echo [LOG] Scene manifests:    Projects\!TARGET_PROJECT!\Assets\Cooked\SceneManifests\
echo [LOG] Mesh assets:        Projects\!TARGET_PROJECT!\Assets\Cooked\Meshes\
echo [LOG] Material assets:    Projects\!TARGET_PROJECT!\Assets\Cooked\Materials\
echo [LOG] Texture assets:     Projects\!TARGET_PROJECT!\Assets\Cooked\Textures\

set "EXIT_RC=0"
goto :FINISH

:MISSING_ARGS
echo [ERROR] Missing required arguments.
echo         CookScene.bat requires both ProjectName and ScenePath.
echo         Example: Scripts\CookScene.bat Showcase Sponza\Sponza.gltf Debug
goto :USAGE

:USAGE
echo.
echo Usage: Scripts\CookScene.bat ^<ProjectName^> ^<ScenePath^> [Debug^|Release^|RelWithDebInfo]
echo.
echo   ProjectName - Project directory under Projects\, for example Showcase
echo   ScenePath   - Path relative to Projects\^<ProjectName^>\Assets\Meshes\
echo   Run from    - Repository root, or call the script with its full path
echo.
echo Examples:
echo   Scripts\CookScene.bat Showcase Sponza\Sponza.gltf
echo   Scripts\CookScene.bat Showcase Bistro\BistroExterior.fbx Release
echo.
set "EXIT_RC=1"

:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%"

echo.
if "%EXIT_RC%"=="0" (
    echo [LOG] Scene cooking workflow completed successfully.
) else (
    echo [ERROR] Scene cooking workflow failed.
)
echo [LOG] Logs: %LOGFILE%
if not defined PARENT_BATCH pause
exit /B %EXIT_RC%