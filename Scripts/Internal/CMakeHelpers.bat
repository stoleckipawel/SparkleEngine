@echo off
:: ============================================================================
:: CMakeHelpers.bat - Shared CMake configure and build helpers
:: ============================================================================
:: Internal helper module used by public scripts to keep CMake invocation,
:: generator selection, and target builds consistent.
::
:: Usage:
::   call "Internal\CMakeHelpers.bat" Configure
::   call "Internal\CMakeHelpers.bat" BuildTargets <Configuration> <Target...>
::
:: Outputs:
::   CMAKE_HELPERS_RC - Return code from the requested operation
:: ============================================================================

setlocal enabledelayedexpansion

if /I "%~1"=="Configure" goto :CONFIGURE
if /I "%~1"=="BuildTargets" goto :BUILD_TARGETS

echo [ERROR] CMakeHelpers.bat requires a valid command.
echo         Supported commands: Configure, BuildTargets
set "HELPER_RC=1"
goto :FINISH

:CONFIGURE
call "%~dp0Config.bat"

if not exist "!BUILD_DIR!" (
    echo [LOG] Creating build directory: !BUILD_DIR!
    mkdir "!BUILD_DIR!"
    if errorlevel 1 (
        echo [ERROR] Failed to create build directory.
        set "HELPER_RC=1"
        goto :FINISH
    )
)

pushd "!BUILD_DIR!"
if "!USE_CLANG!"=="1" (
    echo [LOG] CMake: -G "!GENERATOR!" -A !ARCH! -T ClangCL -Wno-dev
    cmake -G "!GENERATOR!" -A !ARCH! -T ClangCL -Wno-dev "!ROOT_DIR!"
) else (
    echo [LOG] CMake: -G "!GENERATOR!" -A !ARCH! -Wno-dev
    cmake -G "!GENERATOR!" -A !ARCH! -Wno-dev "!ROOT_DIR!"
)
set "HELPER_RC=!ERRORLEVEL!"
popd
goto :FINISH

:BUILD_TARGETS
call "%~dp0Config.bat"

set "BUILD_CONFIG=%~2"
if "%BUILD_CONFIG%"=="" (
    echo [ERROR] BuildTargets requires a configuration.
    set "HELPER_RC=1"
    goto :FINISH
)

shift
shift

set "TARGET_ARGS="
:COLLECT_TARGETS
if "%~1"=="" goto :RUN_BUILD
set "TARGET_ARGS=!TARGET_ARGS! %~1"
shift
goto :COLLECT_TARGETS

:RUN_BUILD
if "!TARGET_ARGS!"=="" (
    echo [ERROR] BuildTargets requires at least one target.
    set "HELPER_RC=1"
    goto :FINISH
)

if not exist "!BUILD_DIR!\CMakeCache.txt" (
    echo [ERROR] CMake cache not found. Run Scripts\GenerateSolution.bat first.
    set "HELPER_RC=1"
    goto :FINISH
)

cmake --build "!BUILD_DIR!" --config !BUILD_CONFIG! --target !TARGET_ARGS! -- /nologo /v:minimal
set "HELPER_RC=!ERRORLEVEL!"
goto :FINISH

:FINISH
set "_TMP_RC=%HELPER_RC%"
endlocal & set "CMAKE_HELPERS_RC=%_TMP_RC%" & exit /B %_TMP_RC%