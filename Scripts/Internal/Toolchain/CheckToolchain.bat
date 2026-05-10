@echo off
:: ============================================================================
:: CheckToolchain.bat - Build toolchain validator
:: ============================================================================
:: Verifies required and optional host build tools are available in PATH.
:: This is an internal helper used by the user-facing workflow scripts.
::
:: Required: PowerShell, CMake, Visual Studio 2022 or newer with C++ tools,
::           Windows SDK, MSBuild, git with sparse-checkout support
:: Optional: ClangCL, clang-format, clang-tidy, git-lfs
::
:: Usage: CheckToolchain.bat [CONTINUE]
::   CONTINUE - Suppress pause (used by parent scripts)
:: ============================================================================

setlocal enabledelayedexpansion

if /I "%~1"=="CONTINUE" if not defined PARENT_BATCH set "PARENT_BATCH=1"

if not defined LOG_CAPTURED (
    call "%~dp0..\Core\BootstrapLog.bat" "%~f0" %*
    set "BOOTSTRAP_RC=!ERRORLEVEL!"
    exit /B !BOOTSTRAP_RC!
)

call "%~dp0..\Core\Config.bat"

set "RC=0"
set "_POWERSHELL_EXE=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "!_POWERSHELL_EXE!" set "_POWERSHELL_EXE=powershell"
set "_VERSION_CHECK_PS1=%~dp0TestVersionAtLeast.ps1"
set "_GIT_REMOTE_CHECK_PS1=%~dp0TestGitRemote.ps1"

echo.
echo ============================================================
echo   Validating Build Toolchain
echo ============================================================
echo.

set "POWERSHELL_AVAILABLE=0"
if exist "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" set "POWERSHELL_AVAILABLE=1"
if "!POWERSHELL_AVAILABLE!"=="0" (
    where powershell >nul 2>&1
    if not errorlevel 1 set "POWERSHELL_AVAILABLE=1"
)
if "!POWERSHELL_AVAILABLE!"=="0" (
    echo [ERROR] Windows PowerShell not found. Script logging, freshness checks, and dependency probes require PowerShell.
    set "RC=1"
) else (
    set "POWERSHELL_VERSION="
    for /f "delims=" %%V in ('"!_POWERSHELL_EXE!" -NoProfile -Command "$PSVersionTable.PSVersion.ToString()" 2^>nul') do (
        if not defined POWERSHELL_VERSION set "POWERSHELL_VERSION=%%V"
    )
    if defined POWERSHELL_VERSION (
        echo [OK] Windows PowerShell !POWERSHELL_VERSION!
    ) else (
        echo [OK] Windows PowerShell
    )
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Install CMake and add to PATH.
    echo         Download: https://cmake.org/download/
    set "RC=1"
) else (
    set "CMAKE_VERSION="
    for /f "tokens=3" %%V in ('cmake --version 2^>nul ^| findstr /i /c:"cmake version"') do (
        if not defined CMAKE_VERSION set "CMAKE_VERSION=%%V"
    )
    if defined CMAKE_VERSION if exist "!_VERSION_CHECK_PS1!" if "!POWERSHELL_AVAILABLE!"=="1" (
        "!_POWERSHELL_EXE!" -NoProfile -ExecutionPolicy Bypass -File "!_VERSION_CHECK_PS1!" -Actual "!CMAKE_VERSION!" -Minimum "!CMAKE_MINIMUM_VERSION!" >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] CMake !CMAKE_VERSION! found, but Sparkle requires CMake !CMAKE_MINIMUM_VERSION! or newer.
            echo         Download: https://cmake.org/download/
            set "RC=1"
        ) else (
            echo [OK] CMake !CMAKE_VERSION! ^(minimum !CMAKE_MINIMUM_VERSION!^)
        )
    ) else (
        echo [OK] CMake !CMAKE_VERSION!
    )

    if not defined GENERATOR (
        echo [ERROR] No compatible Visual Studio CMake generator was resolved.
        echo         Install Visual Studio 2022 or newer with Desktop development with C++ and update CMake if needed.
        set "RC=1"
    ) else (
        cmake --help 2>nul | findstr /c:"!GENERATOR!" >nul
        if errorlevel 1 (
            echo [ERROR] CMake does not expose the resolved generator: !GENERATOR!
            echo         Install or update CMake so it supports your installed Visual Studio version.
            set "RC=1"
        ) else (
            echo [OK] CMake generator !GENERATOR!
        )
    )
)

if not exist "!VSWHERE_EXE!" (
    echo [ERROR] vswhere not found. Install Visual Studio 2022 or newer with the C++ workload.
    echo         Run Visual Studio Installer ^> Modify ^> Desktop development with C++.
    set "RC=1"
) else (
    "!VSWHERE_EXE!" -latest -products * -version "!VS_VERSION_RANGE!" -requires "!VS_CPP_COMPONENT!" -property installationPath >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Visual Studio C++ tools not found for version range !VS_VERSION_RANGE!.
        echo         Run Visual Studio Installer ^> Modify ^> Desktop development with C++.
        set "RC=1"
    ) else (
        if defined VS_INSTALLATION_VERSION (
            echo [OK] !VS_DISPLAY_NAME! C++ tools ^(!VS_INSTALLATION_VERSION!^)
        ) else (
            echo [OK] Visual Studio C++ tools
        )
    )
)

set "MSBUILD_EXE="
where msbuild >nul 2>&1
if not errorlevel 1 set "MSBUILD_EXE=msbuild"
if not defined MSBUILD_EXE if defined VS_INSTALLATION_PATH if exist "!VS_INSTALLATION_PATH!\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_EXE=!VS_INSTALLATION_PATH!\MSBuild\Current\Bin\MSBuild.exe"
)
if not defined MSBUILD_EXE if exist "!VSWHERE_EXE!" (
    for /f "usebackq delims=" %%I in (`""!VSWHERE_EXE!" -latest -products * -version "!VS_VERSION_RANGE!" -requires "!VS_CPP_COMPONENT!" -find MSBuild\Current\Bin\MSBuild.exe"`) do (
        if not defined MSBUILD_EXE set "MSBUILD_EXE=%%~fI"
    )
)
if defined MSBUILD_EXE (
    echo [OK] MSBuild
) else (
    echo [ERROR] MSBuild not found in PATH or the resolved Visual Studio installation.
    echo         Run Visual Studio Installer ^> Modify ^> Desktop development with C++.
    set "RC=1"
)

set "WINDOWS_SDK_VERSION="
set "WINDOWS_SDK_INCLUDE_DIR=%ProgramFiles(x86)%\Windows Kits\10\Include"
if exist "!WINDOWS_SDK_INCLUDE_DIR!" (
    for /f "usebackq delims=" %%V in (`dir /b /ad "!WINDOWS_SDK_INCLUDE_DIR!" 2^>nul ^| sort /r`) do (
        if not defined WINDOWS_SDK_VERSION set "WINDOWS_SDK_VERSION=%%V"
    )
)
if defined WINDOWS_SDK_VERSION (
    echo [OK] Windows SDK !WINDOWS_SDK_VERSION!
) else (
    echo [ERROR] Windows SDK not found. D3D12 builds require a Windows 10/11 SDK.
    echo         Run Visual Studio Installer ^> Modify ^> Desktop development with C++ ^> Windows SDK.
    set "RC=1"
)

if /I "!CMAKE_TOOLSET!"=="ClangCL" (
    where clang-cl >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] SPARKLE_USE_CLANGCL=1 or SPARKLE_CMAKE_TOOLSET=ClangCL was requested, but clang-cl was not found.
        set "RC=1"
    ) else (
        echo [OK] clang-cl ^(requested CMake toolset^)
    )
) else (
    where clang-cl >nul 2>&1
    if errorlevel 1 (
        echo [INFO] clang-cl not found. Configure will use the default MSVC toolset.
    ) else (
        echo [OK] clang-cl ^(available; set SPARKLE_USE_CLANGCL=1 to request ClangCL^)
    )
)

where clang-format >nul 2>&1
if errorlevel 1 (
    echo [WARN] clang-format not found. Formatting command will be unavailable.
) else (
    echo [OK] clang-format
)

where clang-tidy >nul 2>&1
if errorlevel 1 (
    echo [WARN] clang-tidy not found. Static analysis command will be unavailable.
) else (
    echo [OK] clang-tidy
)

where git >nul 2>&1
if errorlevel 1 (
    echo [ERROR] git not found. Required for FetchContent cloning.
    echo         Download: https://git-scm.com/download/win
    set "RC=1"
) else (
    set "GIT_VERSION="
    for /f "tokens=3" %%V in ('git --version 2^>nul') do (
        if not defined GIT_VERSION set "GIT_VERSION=%%V"
    )
    if defined GIT_VERSION if exist "!_VERSION_CHECK_PS1!" if "!POWERSHELL_AVAILABLE!"=="1" (
        "!_POWERSHELL_EXE!" -NoProfile -ExecutionPolicy Bypass -File "!_VERSION_CHECK_PS1!" -Actual "!GIT_VERSION!" -Minimum "!GIT_MINIMUM_VERSION!" >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] git !GIT_VERSION! found, but Sparkle requires git !GIT_MINIMUM_VERSION! or newer for FetchContent sparse checkouts.
            echo         Download: https://git-scm.com/download/win
            set "RC=1"
        ) else (
            echo [OK] git !GIT_VERSION! ^(minimum !GIT_MINIMUM_VERSION!^)
        )
    ) else (
        echo [OK] git !GIT_VERSION!
    )

    git sparse-checkout -h 2>&1 | findstr /i /c:"usage: git sparse-checkout" >nul
    if errorlevel 1 (
        echo [ERROR] git sparse-checkout is unavailable. Update Git for Windows.
        set "RC=1"
    ) else (
        echo [OK] git sparse-checkout
    )
)

set "NEEDS_NETWORK_CHECK=0"
if not exist "!DEPS_DIR!" set "NEEDS_NETWORK_CHECK=1"
if exist "!DEPS_DIR!" (
    dir /b "!DEPS_DIR!" 2>nul | findstr "." >nul 2>&1
    if errorlevel 1 set "NEEDS_NETWORK_CHECK=1"
)
if /I "!SPARKLE_SKIP_NETWORK_CHECK!"=="1" (
    echo [INFO] GitHub reachability check skipped ^(SPARKLE_SKIP_NETWORK_CHECK=1^).
) else if "!NEEDS_NETWORK_CHECK!"=="1" (
    if "!POWERSHELL_AVAILABLE!"=="1" if exist "!_GIT_REMOTE_CHECK_PS1!" (
        "!_POWERSHELL_EXE!" -NoProfile -ExecutionPolicy Bypass -File "!_GIT_REMOTE_CHECK_PS1!" -GitCommand git -RemoteUrl "https://github.com/ocornut/imgui.git" -TimeoutSeconds 15 >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] GitHub dependency source is not reachable from this shell.
            echo         Fresh setup needs GitHub access for CMake FetchContent. Check network, proxy, or run with a populated build\_deps cache.
            set "RC=1"
        ) else (
            echo [OK] GitHub dependency source reachable
        )
    ) else (
        echo [WARN] GitHub reachability check unavailable because PowerShell helper support is missing.
    )
) else (
    echo [OK] Dependency cache present ^(GitHub reachability not required for this run^)
)

where git-lfs >nul 2>&1
if errorlevel 1 (
    echo [INFO] git-lfs not found. Not required ^(LFS downloads are skipped during fetch^).
) else (
    echo [OK] git-lfs
)

echo.
echo ============================================================
if %RC%==0 (
    echo   [SUCCESS] All required build tools found.
) else (
    echo   [ERROR] Missing required build tools. Configure and build steps will fail.
)
echo ============================================================
echo.
echo [LOG] Third-party dependency sync runs during GenerateSolution.bat.

set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "TOOLCHAIN_RC=%_TMP_RC%"

if defined PARENT_BATCH (
    exit /B %TOOLCHAIN_RC%
)

echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %TOOLCHAIN_RC%
