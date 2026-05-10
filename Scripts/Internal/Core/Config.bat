@echo off
:: ============================================================================
:: Config.bat - Shared path and tool configuration
:: ============================================================================
:: Central configuration module called by every top-level script after
:: BootstrapLog. Provides a single source of truth for all paths, tool
:: detection, and build settings.
::
:: Usage: call "%~dp0Internal\Core\Config.bat"   (from Scripts/)
::        call "%~dp0..\Core\Config.bat"          (from Scripts/Internal/<Category>/)
::
:: Sets:
::   ROOT_DIR      - Repository root (parent of Scripts/)
::   BUILD_DIR     - CMake build directory (ROOT_DIR\build)
::   BIN_DIR       - Output binaries (ROOT_DIR\build\bin)
::   LOG_DIR       - Structured log root (ROOT_DIR\logs)
::   PROJECTS_DIR  - User projects (ROOT_DIR\Projects)
::   DEPS_DIR      - FetchContent dependencies (BUILD_DIR\_deps)
::   ENGINE_DIR    - Engine source (ROOT_DIR\Engine)
::   SCRIPTS_DIR   - Scripts directory (ROOT_DIR\Scripts)
::   CMAKE_DIR     - Repo CMake modules root (ROOT_DIR\CMake)
::   GENERATOR     - Required CMake generator (Visual Studio 18 2026)
::   ARCH          - Target architecture (x64)
::   VSWHERE_EXE   - Visual Studio Installer discovery tool
::   VS_VERSION_RANGE - Required Visual Studio major version range
::   VS_CPP_COMPONENT - Required Visual Studio C++ workload component
::   USE_CLANG     - 1 if Clang available, 0 otherwise
::   PROJECT_NAME  - Project name from root CMakeLists.txt project() call
::   SOLUTION_FILE - Full path to the VS solution file
:: ============================================================================

:: ---------------------------------------------------------------------------
:: Resolve repository root
:: ---------------------------------------------------------------------------
:: This script lives in Scripts\Internal\Core\, so repo root is three levels up.
for %%I in ("%~dp0..\..\..") do set "ROOT_DIR=%%~fI"

:: ---------------------------------------------------------------------------
:: Directory paths (all derived from ROOT_DIR)
:: ---------------------------------------------------------------------------
set "BUILD_DIR=!ROOT_DIR!\build"
set "BIN_DIR=!BUILD_DIR!\bin"
set "LOG_DIR=!ROOT_DIR!\logs"
set "PROJECTS_DIR=!ROOT_DIR!\Projects"
set "DEPS_DIR=!BUILD_DIR!\_deps"
set "ENGINE_DIR=!ROOT_DIR!\Engine"
set "SCRIPTS_DIR=!ROOT_DIR!\Scripts"
set "CMAKE_DIR=!ROOT_DIR!\CMake"
set "CMAKE_DEPENDENCIES_DIR=!CMAKE_DIR!\Dependencies"
set "CMAKE_VALIDATION_DIR=!CMAKE_DIR!\Validation"

:: ---------------------------------------------------------------------------
:: Build settings
:: ---------------------------------------------------------------------------
set "GENERATOR=Visual Studio 18 2026"
set "ARCH=x64"
set "VSWHERE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_VERSION_RANGE=[18.0,19.0)"
set "VS_CPP_COMPONENT=Microsoft.VisualStudio.Component.VC.Tools.x86.x64"

:: ---------------------------------------------------------------------------
:: Toolset detection
:: ---------------------------------------------------------------------------
set "USE_CLANG=0"
where clang >nul 2>&1
if not errorlevel 1 set "USE_CLANG=1"

:: ---------------------------------------------------------------------------
:: Project name (extracted from root CMakeLists.txt project() call)
:: ---------------------------------------------------------------------------
:: Provides PROJECT_NAME and SOLUTION_FILE so callers don't duplicate this.
set "PROJECT_NAME="
if exist "!ROOT_DIR!\CMakeLists.txt" (
    for /f "tokens=2 delims=( " %%P in ('findstr /i "project(" "!ROOT_DIR!\CMakeLists.txt"') do (
        set "_RAW_NAME=%%P"
    )
    for /f "delims=) tokens=1" %%A in ("!_RAW_NAME!") do set "PROJECT_NAME=%%A"
    if defined PROJECT_NAME set "PROJECT_NAME=!PROJECT_NAME: =!"
    set "_RAW_NAME="
)

:: Fallback if extraction failed (missing CMakeLists.txt or unexpected format)
if "!PROJECT_NAME!"=="" (
    echo [WARN] Could not extract project name from CMakeLists.txt.
    echo        Falling back to default: Sparkle
    set "PROJECT_NAME=Sparkle"
)

set "SOLUTION_FILE=!BUILD_DIR!\!PROJECT_NAME!.slnx"

