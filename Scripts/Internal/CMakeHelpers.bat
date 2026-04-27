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

set "DESIRED_GENERATOR=!GENERATOR!"
set "DESIRED_PLATFORM=!ARCH!"
set "DESIRED_TOOLSET="
if "!USE_CLANG!"=="1" set "DESIRED_TOOLSET=ClangCL"

if not exist "!BUILD_DIR!" (
    echo [LOG] Creating build directory: !BUILD_DIR!
    mkdir "!BUILD_DIR!"
    if errorlevel 1 (
        echo [ERROR] Failed to create build directory.
        set "HELPER_RC=1"
        goto :FINISH
    )
)

if exist "!BUILD_DIR!\CMakeCache.txt" (
    set "CACHE_GENERATOR="
    set "CACHE_PLATFORM="
    set "CACHE_TOOLSET="
    set "RESET_BUILD_CACHE=0"

    for /f "tokens=2 delims==" %%A in ('findstr /b /c:"CMAKE_GENERATOR:INTERNAL=" "!BUILD_DIR!\CMakeCache.txt"') do set "CACHE_GENERATOR=%%A"
    for /f "tokens=2 delims==" %%A in ('findstr /b /c:"CMAKE_GENERATOR_PLATFORM:INTERNAL=" "!BUILD_DIR!\CMakeCache.txt"') do set "CACHE_PLATFORM=%%A"
    for /f "tokens=2 delims==" %%A in ('findstr /b /c:"CMAKE_GENERATOR_TOOLSET:INTERNAL=" "!BUILD_DIR!\CMakeCache.txt"') do set "CACHE_TOOLSET=%%A"

    if /I not "!CACHE_GENERATOR!"=="!DESIRED_GENERATOR!" set "RESET_BUILD_CACHE=1"
    if /I not "!CACHE_PLATFORM!"=="!DESIRED_PLATFORM!" set "RESET_BUILD_CACHE=1"
    if /I not "!CACHE_TOOLSET!"=="!DESIRED_TOOLSET!" set "RESET_BUILD_CACHE=1"

    if "!RESET_BUILD_CACHE!"=="1" (
        echo [LOG] Detected stale CMake cache settings.
        echo [LOG] Existing: generator='!CACHE_GENERATOR!' platform='!CACHE_PLATFORM!' toolset='!CACHE_TOOLSET!'
        echo [LOG] Desired:  generator='!DESIRED_GENERATOR!' platform='!DESIRED_PLATFORM!' toolset='!DESIRED_TOOLSET!'
        echo [LOG] Clearing CMake cache and generator files before reconfigure...

        if exist "!BUILD_DIR!\CMakeCache.txt" del /q "!BUILD_DIR!\CMakeCache.txt"
        if exist "!BUILD_DIR!\CMakeFiles" rmdir /s /q "!BUILD_DIR!\CMakeFiles"
        if exist "!BUILD_DIR!\*.sln" del /q "!BUILD_DIR!\*.sln"
        if exist "!BUILD_DIR!\*.vcxproj" del /q "!BUILD_DIR!\*.vcxproj"
        if exist "!BUILD_DIR!\*.vcxproj.filters" del /q "!BUILD_DIR!\*.vcxproj.filters"
        if exist "!BUILD_DIR!\ZERO_CHECK.vcxproj" del /q "!BUILD_DIR!\ZERO_CHECK.vcxproj"
        if exist "!BUILD_DIR!\ZERO_CHECK.vcxproj.filters" del /q "!BUILD_DIR!\ZERO_CHECK.vcxproj.filters"
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

set "MSBUILD_ARGS=/nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false"
if defined SPARKLE_MSBUILD_ARGS set "MSBUILD_ARGS=!SPARKLE_MSBUILD_ARGS!"

echo [LOG] MSBuild args: !MSBUILD_ARGS!
cmake --build "!BUILD_DIR!" --config !BUILD_CONFIG! --target !TARGET_ARGS! -- !MSBUILD_ARGS!
set "HELPER_RC=!ERRORLEVEL!"
goto :FINISH

:FINISH
set "_TMP_RC=%HELPER_RC%"
endlocal & set "CMAKE_HELPERS_RC=%_TMP_RC%" & exit /B %_TMP_RC%