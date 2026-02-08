@echo off
:: ============================================================================
:: Config.bat - Shared path and tool configuration
:: ============================================================================
:: Central configuration module called by every top-level script after
:: BootstrapLog. Provides a single source of truth for all paths, tool
:: detection, and build settings.
::
:: Usage: call "%~dp0Internal\Config.bat"   (from Scripts/)
::        call "%~dp0Config.bat"            (from Scripts/Internal/)
::
:: Sets:
::   ROOT_DIR      - Repository root (parent of Scripts/)
::   BUILD_DIR     - CMake build directory (ROOT_DIR\build)
::   BIN_DIR       - Output binaries (ROOT_DIR\bin)
::   PROJECTS_DIR  - User projects (ROOT_DIR\Projects)
::   DEPS_DIR      - FetchContent dependencies (BUILD_DIR\_deps)
::   ENGINE_DIR    - Engine source (ROOT_DIR\Engine)
::   SCRIPTS_DIR   - Scripts directory (ROOT_DIR\Scripts)
::   GENERATOR     - CMake generator (Visual Studio 17 2022)
::   ARCH          - Target architecture (x64)
::   USE_CLANG     - 1 if Clang available, 0 otherwise
::   PROJECT_NAME  - Project name from root CMakeLists.txt project() call
::   SOLUTION_FILE - Full path to the VS solution file
:: ============================================================================

:: ---------------------------------------------------------------------------
:: Resolve repository root
:: ---------------------------------------------------------------------------
:: This script lives in Scripts\Internal\, so repo root is two levels up.
for %%I in ("%~dp0..\..") do set "ROOT_DIR=%%~fI"

:: ---------------------------------------------------------------------------
:: Directory paths (all derived from ROOT_DIR)
:: ---------------------------------------------------------------------------
set "BUILD_DIR=!ROOT_DIR!\build"
set "BIN_DIR=!ROOT_DIR!\bin"
set "PROJECTS_DIR=!ROOT_DIR!\Projects"
set "DEPS_DIR=!BUILD_DIR!\_deps"
set "ENGINE_DIR=!ROOT_DIR!\Engine"
set "SCRIPTS_DIR=!ROOT_DIR!\Scripts"

:: ---------------------------------------------------------------------------
:: Build settings
:: ---------------------------------------------------------------------------
set "GENERATOR=Visual Studio 17 2022"
set "ARCH=x64"

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
    set "PROJECT_NAME=!PROJECT_NAME: =!"
    set "_RAW_NAME="
)

:: Fallback if extraction failed (missing CMakeLists.txt or unexpected format)
if "!PROJECT_NAME!"=="" (
    echo [WARN] Could not extract project name from CMakeLists.txt.
    echo        Falling back to default: Sparkle
    set "PROJECT_NAME=Sparkle"
)

set "SOLUTION_FILE=!BUILD_DIR!\!PROJECT_NAME!.sln"

