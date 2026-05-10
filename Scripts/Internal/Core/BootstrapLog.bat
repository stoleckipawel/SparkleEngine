@echo off
:: ============================================================================
:: BootstrapLog.bat - Centralized logging bootstrap for build toolchain
:: ============================================================================
:: Creates timestamped action log files and captures all script output via
:: PowerShell. Invoked by top-level scripts to ensure consistent logging.
::
:: Usage: call "BootstrapLog.bat" "<caller_full_path>" [args...]
::
:: Environment:
::   LOG_CAPTURED  - Set to "1" when logging is active (prevents re-entry)
::   LOGFILE       - Absolute path to the current session's log file
::   LOG_LATESTFILE - Absolute path to the latest log alias for this action
:: ============================================================================

setlocal enabledelayedexpansion

for %%I in ("%~f0") do set "SELF_DIR=%%~dpI"

:: Guard: Already capturing - nothing to do
if defined LOG_CAPTURED (
    endlocal
    exit /B 0
)

:: Validate caller path argument
if "%~1"=="" (
    echo [ERROR] BootstrapLog.bat requires caller path as first argument.
    endlocal
    exit /B 1
)

set "CALLER=%~1"
set "PRIMARY_ARG=%~2"

:: ---------------------------------------------------------------------------
:: Build remaining arguments (everything after the caller path)
:: ---------------------------------------------------------------------------
:: CRITICAL: In batch, %* does NOT update after shift - it always contains
:: the full original argument list including %1. We must rebuild the remaining
:: args manually to avoid passing the caller path as %1 to the re-invoked script.
set "REMAINING_ARGS="
shift
:COLLECT_ARGS
if "%~1"=="" goto :ARGS_READY
set "REMAINING_ARGS=!REMAINING_ARGS! %1"
shift
goto :COLLECT_ARGS
:ARGS_READY

:: ---------------------------------------------------------------------------
:: Resolve repository root directory
:: ---------------------------------------------------------------------------
:: Prefer the caller's directory if it contains LICENSE.txt (repo root marker).
:: Otherwise fall back to this script's directory (Scripts\Internal\Core).
for %%F in ("%CALLER%") do set "CALLER_DIR=%%~dpF"

set "ROOT_DIR="
if exist "%CALLER_DIR%LICENSE.txt" set "ROOT_DIR=%CALLER_DIR%"
if not defined ROOT_DIR set "ROOT_DIR=!SELF_DIR!..\..\.."
:: Normalize to absolute path with trailing backslash
pushd "%ROOT_DIR%" >nul 2>&1
set "ROOT_DIR=%CD%\"
popd >nul

:: ---------------------------------------------------------------------------
:: Resolve action-specific log location
:: ---------------------------------------------------------------------------
for %%F in ("%CALLER%") do set "CALLER_NAME=%%~nF"

set "LOG_ACTION=GeneralLog"
set "LOG_STEM=%CALLER_NAME%Log"
set "LOG_SCOPE=%PRIMARY_ARG%"

if /I "%CALLER_NAME%"=="SetupWorkspace"    set "LOG_ACTION=WorkspaceSetupLog"     & set "LOG_STEM=WorkspaceSetupLog"     & set "LOG_SCOPE=Workspace"
if /I "%CALLER_NAME%"=="GenerateSolution"  set "LOG_ACTION=SolutionGenerationLog" & set "LOG_STEM=SolutionGenerationLog" & set "LOG_SCOPE=Workspace"
if /I "%CALLER_NAME%"=="BuildProject"      set "LOG_ACTION=BuildLog"              & set "LOG_STEM=BuildLog"
if /I "%CALLER_NAME%"=="CookAllAssets"     set "LOG_ACTION=AssetCookingLog"       & set "LOG_STEM=AssetCookingLog"
if /I "%CALLER_NAME%"=="CookShaders"       set "LOG_ACTION=ShaderCompilationLog"  & set "LOG_STEM=ShaderCompilationLog"
if /I "%CALLER_NAME%"=="CookTextures"      set "LOG_ACTION=TextureCookingLog"     & set "LOG_STEM=TextureCookingLog"
if /I "%CALLER_NAME%"=="CookAssets"        set "LOG_ACTION=AssetCookingLog"       & set "LOG_STEM=AssetCookingLog"
if /I "%CALLER_NAME%"=="RunClangFormat"    set "LOG_ACTION=FormatCheckLog"        & set "LOG_STEM=FormatCheckLog"        & set "LOG_SCOPE=Workspace"
if /I "%CALLER_NAME%"=="CheckToolchain"    set "LOG_ACTION=ToolchainCheckLog"     & set "LOG_STEM=ToolchainCheckLog"     & set "LOG_SCOPE=Workspace"
if /I "%CALLER_NAME%"=="CleanWorkspace"    set "LOG_ACTION=WorkspaceCleanupLog"   & set "LOG_STEM=WorkspaceCleanupLog"   & set "LOG_SCOPE=Workspace"

if not defined LOG_SCOPE set "LOG_SCOPE=Workspace"
if /I "%LOG_SCOPE%"=="ALL" set "LOG_SCOPE=AllProjects"
set "LOG_SCOPE_FIRST=!LOG_SCOPE:~0,1!"
if "!LOG_SCOPE_FIRST!"=="/" set "LOG_SCOPE=Workspace"
if "!LOG_SCOPE_FIRST!"=="-" set "LOG_SCOPE=Workspace"

call :SANITIZE_SEGMENT "%LOG_ACTION%" LOG_ACTION
call :SANITIZE_SEGMENT "%LOG_STEM%" LOG_STEM
call :SANITIZE_SEGMENT "%LOG_SCOPE%" LOG_SCOPE

set "LOG_DIR=%ROOT_DIR%logs\Prerequisites\%LOG_ACTION%\%LOG_SCOPE%"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%" >nul 2>&1

:: Generate timestamp: Day_MM-DD-YYYY__H-MM-SS-CC
set "TS=%DATE%_%TIME%"
set "TS=%TS::=-%"
set "TS=%TS:/=-%"
set "TS=%TS:,=-%"
set "TS=%TS:.=-%"
set "TS=%TS: =_%"
set "LOGFILE=%LOG_DIR%\%LOG_STEM%_%TS%.txt"
set "LOG_LATESTFILE=%LOG_DIR%\Latest.txt"

:: ---------------------------------------------------------------------------
:: Re-invoke caller under the PowerShell logging helper for output capture
:: ---------------------------------------------------------------------------
:: Exports LOG_CAPTURED and LOGFILE so the re-invoked script skips bootstrap.
:: Use cmd.exe's standard batch invocation quoting form:
::   cmd /c ""path\to\script.bat" arg1 arg2"
:: REMAINING_ARGS is used instead of %* to avoid passing the caller path as %1.
set "BOOTSTRAP_HELPER=!SELF_DIR!BootstrapLog.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -File "!BOOTSTRAP_HELPER!" -Caller "!CALLER!" -RemainingArgs "!REMAINING_ARGS!" -LogFile "!LOGFILE!"
set "RC=%ERRORLEVEL%"

:: Copy to a stable "latest" log for easy access within this action folder.
copy /Y "%LOGFILE%" "%LOG_LATESTFILE%" >nul 2>&1

endlocal & set "LOGFILE=%LOGFILE%" & set "LOG_LATESTFILE=%LOG_LATESTFILE%" & exit /B %RC%

:SANITIZE_SEGMENT
set "_SEGMENT=%~1"
if not defined _SEGMENT set "_SEGMENT=Workspace"
set "_SEGMENT=%_SEGMENT:\=_%"
set "_SEGMENT=%_SEGMENT:/=_%"
set "_SEGMENT=%_SEGMENT::=_%"
set "_SEGMENT=%_SEGMENT: =_%"
if not defined _SEGMENT set "_SEGMENT=Workspace"
set "%~2=%_SEGMENT%"
exit /B 0
