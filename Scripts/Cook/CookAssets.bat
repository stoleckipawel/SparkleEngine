@echo off
:: ============================================================================
:: CookAssets.bat - Cook scene, mesh, and material assets for a project
:: ============================================================================
:: Enumerates supported engine/project scenes and cooks scene manifests plus
:: cooked mesh and material outputs through the standalone AssetConverter.
::
:: Usage:
::   CookAssets.bat <ProjectName> [Debug|Release|RelWithDebInfo]
::
:: Examples:
::   CookAssets.bat Showcase
::   CookAssets.bat Showcase Release
:: ============================================================================

setlocal enabledelayedexpansion

if not defined LOG_CAPTURED (
	call "%~dp0..\Internal\BootstrapLog.bat" "%~f0" %*
	set "BOOTSTRAP_RC=!ERRORLEVEL!"
	exit /B !BOOTSTRAP_RC!
)

call "%~dp0..\Internal\Config.bat"

set "EXIT_RC=1"
set "TARGET_PROJECT=%~1"
set "CONFIG=%~2"
if defined TARGET_PROJECT set "TARGET_PROJECT=!TARGET_PROJECT: =!"
if defined CONFIG set "CONFIG=!CONFIG: =!"
set "COOKED_COUNT=0"
set "TOTAL_SCENE_COUNT=0"
set "ENGINE_SCENE_COUNT=0"
set "PROJECT_SCENE_COUNT=0"
set "OVERRIDDEN_ENGINE_COUNT=0"
set "COOK_TEMP_DIR=!BUILD_DIR!\Cook\Temp"
if not exist "!COOK_TEMP_DIR!" mkdir "!COOK_TEMP_DIR!" >nul 2>&1
set "SCENE_LIST_FILE=!COOK_TEMP_DIR!\sparkle-cookassets-%RANDOM%%RANDOM%.txt"
set "SCENE_SUMMARY_FILE=!COOK_TEMP_DIR!\sparkle-cookassets-summary-%RANDOM%%RANDOM%.txt"

if /I "%TARGET_PROJECT%"=="/h" goto :USAGE
if /I "%TARGET_PROJECT%"=="-h" goto :USAGE
if /I "%TARGET_PROJECT%"=="/help" goto :USAGE
if /I "%TARGET_PROJECT%"=="--help" goto :USAGE

call "%~dp0..\Internal\ProjectDiscovery.bat" ListProjects
if errorlevel 1 (
	echo [ERROR] Failed to discover runnable projects.
	set "EXIT_RC=1"
	goto :FINISH
)

if "!PROJECT_COUNT!"=="0" (
	echo [ERROR] No runnable projects found in Projects\.
	echo         Restore Projects\Showcase or add a runnable project under Projects\.
	set "EXIT_RC=1"
	goto :FINISH
)

if not defined TARGET_PROJECT goto :PROJECT_MENU
goto :VALIDATE_PROJECT

:PROJECT_MENU
if defined PARENT_BATCH goto :USAGE

echo.
echo ============================================================
echo   Select Project To Cook Scene Assets
echo ============================================================
echo.
for /L %%I in (1,1,!PROJECT_COUNT!) do (
	call echo   %%I^) %%PROJECT_%%I%%
)
echo.
echo ============================================================

set "PROJ_SEL="
set /P "PROJ_SEL=Enter choice [1-!PROJECT_COUNT!]: "
if "!PROJ_SEL!"=="" set "PROJ_SEL=1"

set "VALID_SEL=0"
for /L %%I in (1,1,!PROJECT_COUNT!) do (
	if "!PROJ_SEL!"=="%%I" set "VALID_SEL=1"
)
if "!VALID_SEL!"=="0" (
	echo [ERROR] Invalid selection: '!PROJ_SEL!'.
	goto :PROJECT_MENU
)

for /L %%I in (1,1,!PROJECT_COUNT!) do (
	if "%%I"=="!PROJ_SEL!" set "TARGET_PROJECT=!PROJECT_%%I!"
)

:VALIDATE_PROJECT
set "PROJECT_FOUND=0"
for /L %%I in (1,1,!PROJECT_COUNT!) do (
	if /I "!TARGET_PROJECT!"=="!PROJECT_%%I!" set "PROJECT_FOUND=1"
)

if "!PROJECT_FOUND!"=="0" (
	echo [ERROR] Unknown project '!TARGET_PROJECT!'.
	goto :USAGE
)

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

set "PROJECT_MESH_ROOT=!PROJECT_ROOT!\Assets\Meshes"
set "ENGINE_MESH_ROOT=!ENGINE_DIR!\Assets\Meshes"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\Internal\EnumerateCookScenes.ps1" ^
	-EngineRoot "!ENGINE_MESH_ROOT!" ^
	-ProjectRoot "!PROJECT_MESH_ROOT!" ^
	-ListFile "!SCENE_LIST_FILE!" ^
	-SummaryFile "!SCENE_SUMMARY_FILE!"
if errorlevel 1 (
	echo [ERROR] Failed to enumerate asset source scenes.
	set "EXIT_RC=1"
	goto :FINISH
)

for /f "tokens=1,* delims==" %%A in ('type "!SCENE_SUMMARY_FILE!"') do (
	set "%%A=%%B"
)

if "!TOTAL_SCENE_COUNT!"=="0" (
	echo [ERROR] No supported source scenes were found under:
	echo         !ENGINE_MESH_ROOT!
	echo         !PROJECT_MESH_ROOT!
	echo         Expected one or more .gltf, .glb, or .fbx files.
	set "EXIT_RC=1"
	goto :FINISH
)

echo [LOG] Project: !TARGET_PROJECT!
echo [LOG] Configuration: !CONFIG!
echo [LOG] Engine mesh root:  !ENGINE_MESH_ROOT!
echo [LOG] Project mesh root: !PROJECT_MESH_ROOT!
echo [LOG] Engine scenes discovered:  !ENGINE_SCENE_COUNT!
echo [LOG] Project scenes discovered: !PROJECT_SCENE_COUNT!
echo [LOG] Project overrides applied: !OVERRIDDEN_ENGINE_COUNT!
echo [LOG] Final scenes to cook:      !TOTAL_SCENE_COUNT!

call "%~dp0..\Internal\AssetCooking.bat" PrepareAssetConverter !CONFIG!
if errorlevel 1 (
	set "EXIT_RC=1"
	goto :FINISH
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\Internal\InvokeCookSceneList.ps1" ^
	-ProjectRoot "!PROJECT_ROOT!" ^
	-SceneListFile "!SCENE_LIST_FILE!" ^
	-AssetConverterExe "!ASSET_CONVERTER_EXE!" ^
	-TotalSceneCount !TOTAL_SCENE_COUNT!
if errorlevel 1 (
	set "EXIT_RC=1"
	goto :FINISH
)

set "COOKED_COUNT=!TOTAL_SCENE_COUNT!"

echo.
echo [SUCCESS] CookAssets completed successfully.
echo [LOG] Cooked scene inputs: !COOKED_COUNT!
echo [LOG] Scene manifests:    build\Cooked\!TARGET_PROJECT!\SceneManifests\
echo [LOG] Mesh assets:        build\Cooked\!TARGET_PROJECT!\Meshes\
echo [LOG] Material assets:    build\Cooked\!TARGET_PROJECT!\Materials\

set "EXIT_RC=0"
goto :FINISH

:USAGE
echo.
echo Usage: Scripts\Cook\CookAssets.bat ^<ProjectName^> [Debug^|Release^|RelWithDebInfo]
echo.
echo Examples:
echo   Scripts\Cook\CookAssets.bat Showcase
echo   Scripts\Cook\CookAssets.bat Showcase Release
echo.
echo This command cooks scene manifests plus cooked mesh and material assets
echo from supported source scenes under Engine\Assets\Meshes and
echo Projects\^<ProjectName^>\Assets\Meshes.
echo.
set "EXIT_RC=1"

:FINISH
if exist "%SCENE_LIST_FILE%" del /q "%SCENE_LIST_FILE%" >nul 2>&1
if exist "%SCENE_SUMMARY_FILE%" del /q "%SCENE_SUMMARY_FILE%" >nul 2>&1
endlocal & exit /B %EXIT_RC%