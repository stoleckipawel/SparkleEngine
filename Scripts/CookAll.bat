@echo off
:: ============================================================================
:: CookAll.bat - Build and run the AssetConverter for every project scene
:: ============================================================================
:: Makes full-project scene cooking a normal contributor workflow:
::   1. Validates required tools and third-party dependencies
::   2. Generates the solution if needed
::   3. Builds the AssetConverter target for the requested configuration
::   4. Recursively cooks all supported scenes under Assets\Meshes\
::
:: Usage:
::   CookAll.bat <ProjectName> [Configuration]
::
:: Arguments:
::   ProjectName    - Project directory under Projects\, for example Showcase
::   Configuration  - Debug | Release | RelWithDebInfo (default: Debug)
::
:: Supported source scene extensions:
::   .gltf .glb .fbx
::
:: Example:
::   CookAll.bat Showcase Debug
:: ============================================================================

setlocal enabledelayedexpansion

if not defined LOG_CAPTURED (
    call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
    exit /B %ERRORLEVEL%
)

call "%~dp0Internal\Config.bat"

set "EXIT_RC=1"
set "TARGET_PROJECT=%~1"
set "CONFIG=%~2"
set "FAILED_SCENES="
set "FAILED_COUNT=0"
set "COOKED_COUNT=0"
set "SCENE_COUNT=0"
set "SCENE_LIST_FILE=%TEMP%\sparkle-cookall-%RANDOM%%RANDOM%.txt"

if /I "%TARGET_PROJECT%"=="/h" goto :USAGE
if /I "%TARGET_PROJECT%"=="-h" goto :USAGE
if /I "%TARGET_PROJECT%"=="/help" goto :USAGE
if /I "%TARGET_PROJECT%"=="--help" goto :USAGE

if "%TARGET_PROJECT%"=="" goto :MISSING_ARGS
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

set "SCENE_MESH_ROOT=!PROJECT_ROOT!\Assets\Meshes"
if not exist "!SCENE_MESH_ROOT!" (
    echo [ERROR] Scene mesh root was not found.
    echo         Expected: !SCENE_MESH_ROOT!
    set "EXIT_RC=1"
    goto :FINISH
)

powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-ChildItem -Path '!SCENE_MESH_ROOT!' -Recurse -File -Include *.gltf,*.glb,*.fbx | ForEach-Object { $_.FullName }" > "!SCENE_LIST_FILE!"
if errorlevel 1 (
    echo [ERROR] Failed to enumerate scene source files under '!SCENE_MESH_ROOT!'.
    set "EXIT_RC=1"
    goto :FINISH
)

for /f "usebackq delims=" %%F in ("!SCENE_LIST_FILE!") do (
    set /A SCENE_COUNT+=1
)

if "!SCENE_COUNT!"=="0" (
    echo [ERROR] No supported scene source files were found under '!SCENE_MESH_ROOT!'.
    echo         Expected one or more .gltf, .glb, or .fbx files.
    set "EXIT_RC=1"
    goto :FINISH
)

echo [LOG] Project: !TARGET_PROJECT!
echo [LOG] Configuration: !CONFIG!
echo [LOG] Scene root: !SCENE_MESH_ROOT!
echo [LOG] Supported scene count: !SCENE_COUNT!

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
echo [LOG] Step 4/4: Building AssetConverter and cooking all scenes...
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
for /f "usebackq delims=" %%F in ("!SCENE_LIST_FILE!") do (
    set "CURRENT_SCENE_PATH=%%~fF"
    set "CURRENT_SCENE_REL=!CURRENT_SCENE_PATH:%SCENE_MESH_ROOT%\=!"
    echo.
    echo [LOG] Cooking [!COOKED_COUNT!/!SCENE_COUNT!]: !CURRENT_SCENE_REL!
    "!ASSET_CONVERTER_EXE!" "!CURRENT_SCENE_PATH!"
    set "COOK_RC=!ERRORLEVEL!"
    if "!COOK_RC!" NEQ "0" (
        echo [ERROR] Failed to cook '!CURRENT_SCENE_REL!'.
        set /A FAILED_COUNT+=1
        set "FAILED_SCENES=!FAILED_SCENES!!CURRENT_SCENE_REL!;"
    ) else (
        set /A COOKED_COUNT+=1
    )
)
popd

if "!FAILED_COUNT!" NEQ "0" (
    echo.
    echo [ERROR] CookAll completed with !FAILED_COUNT! failed scene(s).
    echo [ERROR] Failed scenes: !FAILED_SCENES!
    set "EXIT_RC=1"
    goto :FINISH
)

echo.
echo [SUCCESS] CookAll completed successfully.
echo [LOG] Cooked scenes:      !COOKED_COUNT!
echo [LOG] Cooked output root: Projects\!TARGET_PROJECT!\Assets\Cooked\
echo [LOG] Scene manifests:    Projects\!TARGET_PROJECT!\Assets\Cooked\SceneManifests\
echo [LOG] Mesh assets:        Projects\!TARGET_PROJECT!\Assets\Cooked\Meshes\
echo [LOG] Material assets:    Projects\!TARGET_PROJECT!\Assets\Cooked\Materials\
echo [LOG] Texture assets:     Projects\!TARGET_PROJECT!\Assets\Cooked\Textures\

set "EXIT_RC=0"
goto :FINISH

:MISSING_ARGS
echo [ERROR] Missing required arguments.
echo         CookAll.bat requires ProjectName.
echo         Example: Scripts\CookAll.bat Showcase Debug
goto :USAGE

:USAGE
echo.
echo Usage: Scripts\CookAll.bat ^<ProjectName^> [Debug^|Release^|RelWithDebInfo]
echo.
echo   ProjectName - Project directory under Projects\, for example Showcase
echo   Run from    - Repository root, or call the script with its full path
echo.
echo Supported scene extensions:
echo   .gltf .glb .fbx
echo.
echo Example:
echo   Scripts\CookAll.bat Showcase Debug
echo.
set "EXIT_RC=1"

:FINISH
if exist "%SCENE_LIST_FILE%" del /Q "%SCENE_LIST_FILE%" >nul 2>&1
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%"

echo.
if "%EXIT_RC%"=="0" (
    echo [LOG] CookAll workflow completed successfully.
) else (
    echo [ERROR] CookAll workflow failed.
)
echo [LOG] Logs: %LOGFILE%
if not defined PARENT_BATCH pause
exit /B %EXIT_RC%