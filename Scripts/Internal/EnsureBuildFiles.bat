@echo off
:: ============================================================================
:: EnsureBuildFiles.bat - Freshness-gated build-file refresh helper
:: ============================================================================
:: Internal helper used by normal build/cook scripts to avoid unconditional
:: CMake regeneration. Set SPARKLE_FORCE_CONFIGURE=1 to force GenerateSolution.
:: ============================================================================

setlocal enabledelayedexpansion

call "%~dp0Config.bat"

set "HELPER_RC=1"
set "DESIRED_TOOLSET="
if "!USE_CLANG!"=="1" set "DESIRED_TOOLSET=ClangCL"

if /I "!SPARKLE_FORCE_CONFIGURE!"=="1" (
    echo [LOG] SPARKLE_FORCE_CONFIGURE=1. Refreshing build files...
    goto :RUN_CONFIGURE
)

call :CHECK_CURRENT
if "!ERRORLEVEL!"=="0" (
    set "HELPER_RC=0"
    goto :FINISH
)

:RUN_CONFIGURE
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\GenerateSolution.bat" CONTINUE
set "CONFIGURE_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!CONFIGURE_RC!" NEQ "0" (
    echo [ERROR] GenerateSolution step failed.
    set "HELPER_RC=1"
    goto :FINISH
)

call :UPDATE_STAMP
if "!ERRORLEVEL!" NEQ "0" (
    echo [WARN] Build-file freshness stamp update failed. Future scripts may refresh build files again.
)

set "HELPER_RC=0"
goto :FINISH

:CHECK_CURRENT
set "_POWERSHELL_EXE=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%_POWERSHELL_EXE%" set "_POWERSHELL_EXE=powershell"

"%_POWERSHELL_EXE%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0Test-BuildFilesCurrent.ps1" ^
    -RootDir "!ROOT_DIR!" ^
    -BuildDir "!BUILD_DIR!" ^
    -SolutionFile "!SOLUTION_FILE!" ^
    -Generator "!GENERATOR!" ^
    -Platform "!ARCH!" ^
    -Toolset "!DESIRED_TOOLSET!"
exit /B %ERRORLEVEL%

:UPDATE_STAMP
set "_POWERSHELL_EXE=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%_POWERSHELL_EXE%" set "_POWERSHELL_EXE=powershell"

"%_POWERSHELL_EXE%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0Test-BuildFilesCurrent.ps1" ^
    -RootDir "!ROOT_DIR!" ^
    -BuildDir "!BUILD_DIR!" ^
    -SolutionFile "!SOLUTION_FILE!" ^
    -Generator "!GENERATOR!" ^
    -Platform "!ARCH!" ^
    -Toolset "!DESIRED_TOOLSET!" ^
    -UpdateStamp
exit /B %ERRORLEVEL%

:FINISH
set "_TMP_RC=%HELPER_RC%"
endlocal & set "ENSURE_BUILD_FILES_RC=%_TMP_RC%" & exit /B %_TMP_RC%