@echo off
:: ============================================================================
:: CookShaders.bat - Validate and cook shader packages for a project
:: ============================================================================
:: Validates the merged shader package manifest and emits cooked shader
:: packages plus the shader registry for the selected project.
::
:: Usage:
::   CookShaders.bat <ProjectName> [Debug|Release|RelWithDebInfo]
::
:: Examples:
::   CookShaders.bat Showcase
::   CookShaders.bat Showcase Release
:: ============================================================================

setlocal enabledelayedexpansion

if not defined LOG_CAPTURED (
	call "%~dp0..\Internal\BootstrapLog.bat" "%~f0" %*
	exit /B %ERRORLEVEL%
)

call "%~dp0..\Internal\Config.bat"

set "EXIT_RC=1"
set "TARGET_PROJECT=%~1"
set "CONFIG=%~2"
set "TARGET_PROJECT=!TARGET_PROJECT: =!"
set "CONFIG=!CONFIG: =!"

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
	echo         Restore Projects\Showcase or create one using CreateProject.bat.
	set "EXIT_RC=1"
	goto :FINISH
)

if not defined TARGET_PROJECT goto :PROJECT_MENU
goto :VALIDATE_PROJECT

:PROJECT_MENU
if defined PARENT_BATCH goto :USAGE

echo.
echo ============================================================
echo   Select Project To Cook Shaders
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

echo [LOG] Project: !TARGET_PROJECT!
echo [LOG] Configuration: !CONFIG!

call "%~dp0..\Internal\AssetCooking.bat" PrepareShaderCompiler !CONFIG!
if errorlevel 1 (
	set "EXIT_RC=1"
	goto :FINISH
)

echo.
echo [LOG] Validating shader cook manifest required for runtime startup...
pushd "!PROJECT_ROOT!" >nul
"!SHADER_COMPILER_EXE!" inspect-manifest
set "SHADER_MANIFEST_RC=!ERRORLEVEL!"
popd >nul
if "!SHADER_MANIFEST_RC!" NEQ "0" (
	echo [ERROR] Shader cook manifest validation failed. Normal runtime startup requires cooked shader artifacts.
	set "EXIT_RC=1"
	goto :FINISH
)

echo.
echo [LOG] Cooking shader packages required for runtime startup...
pushd "!PROJECT_ROOT!" >nul
"!SHADER_COMPILER_EXE!" cook
set "SHADER_COOK_RC=!ERRORLEVEL!"
popd >nul
if "!SHADER_COOK_RC!" NEQ "0" (
	echo [ERROR] Shader package cooking failed. Runtime startup cannot proceed without cooked shader artifacts.
	set "EXIT_RC=1"
	goto :FINISH
)

echo.
echo [SUCCESS] CookShaders completed successfully.
echo [LOG] Shader packages: Projects\!TARGET_PROJECT!\Assets\Cooked\Shaders\Packages\
echo [LOG] Shader registry: Projects\!TARGET_PROJECT!\Assets\Cooked\Shaders\ShaderPackageRegistry.sreg

set "EXIT_RC=0"
goto :FINISH

:USAGE
echo.
echo Usage: Scripts\Cook\CookShaders.bat ^<ProjectName^> [Debug^|Release^|RelWithDebInfo]
echo.
echo Examples:
echo   Scripts\Cook\CookShaders.bat Showcase
echo   Scripts\Cook\CookShaders.bat Showcase Release
echo.
echo This command validates the merged shader cook manifest for the selected
echo project and emits cooked shader packages plus the shader registry under:
echo   Projects\^<ProjectName^>\Assets\Cooked\Shaders\
echo.
set "EXIT_RC=1"

:FINISH
endlocal & exit /B %EXIT_RC%