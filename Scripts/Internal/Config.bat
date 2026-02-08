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
::   ROOT_DIR     - Repository root (parent of Scripts/)
::   BUILD_DIR    - CMake build directory (ROOT_DIR\build)
::   BIN_DIR      - Output binaries (ROOT_DIR\bin)
::   PROJECTS_DIR - User projects (ROOT_DIR\Projects)
::   DEPS_DIR     - FetchContent dependencies (BUILD_DIR\_deps)
::   ENGINE_DIR   - Engine source (ROOT_DIR\Engine)
::   GENERATOR    - CMake generator
::   ARCH         - Target architecture
::   USE_CLANG    - 1 if Clang available, 0 otherwise
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

