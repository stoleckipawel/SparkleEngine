@echo off
:: ============================================================================
:: CreateProject.bat - Sparkle project generator
:: ============================================================================
:: Creates a new Sparkle project from Projects\TemplateProject.
::
:: Usage: CreateProject.bat [ProjectName]
::   If ProjectName is omitted, prompts interactively.
:: ============================================================================

setlocal enabledelayedexpansion

set "INTERACTIVE=1"
if defined PARENT_BATCH set "INTERACTIVE=0"

if not defined LOG_CAPTURED (
    call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
    exit /B %ERRORLEVEL%
)

call "%~dp0Internal\Config.bat"

set "TEMPLATE_DIR=!PROJECTS_DIR!\TemplateProject"
if not exist "!TEMPLATE_DIR!" (
    echo [ERROR] Template directory not found: !TEMPLATE_DIR!
    echo         Ensure Projects\TemplateProject exists.
    set "EXIT_RC=1"
    goto :FINISH
)

set "PROJECT_NAME=%~1"
set "PROJECT_NAME=!PROJECT_NAME: =!"
if "!INTERACTIVE!"=="0" if "!PROJECT_NAME!"=="" (
    echo [ERROR] CreateProject.bat requires a project name in non-interactive mode.
    set "EXIT_RC=1"
    goto :FINISH
)
if "!PROJECT_NAME!"=="" (
    echo.
    echo ============================================================
    echo   Sparkle Project Generator
    echo ============================================================
    echo.
    set /P "PROJECT_NAME=Enter project name (for example MyGame): "
)

if "!PROJECT_NAME!"=="" (
    echo [ERROR] Project name cannot be empty.
    set "EXIT_RC=1"
    goto :FINISH
)

echo !PROJECT_NAME!| findstr /r "^[A-Za-z_][A-Za-z0-9_]*$" >nul
if errorlevel 1 (
    echo [ERROR] Invalid project name: !PROJECT_NAME!
    echo         Project name must start with a letter or underscore,
    echo         and contain only letters, numbers, and underscores.
    set "EXIT_RC=1"
    goto :FINISH
)

set "PROJECT_DIR=!PROJECTS_DIR!\!PROJECT_NAME!"
if exist "!PROJECT_DIR!" (
    echo [ERROR] Project already exists: !PROJECT_DIR!
    set "EXIT_RC=1"
    goto :FINISH
)

echo.
echo [LOG] Creating project: !PROJECT_NAME!
echo [LOG] Destination: !PROJECT_DIR!
echo.

if not exist "!PROJECTS_DIR!" mkdir "!PROJECTS_DIR!"

echo [LOG] Copying template files...
xcopy /E /I /Q "!TEMPLATE_DIR!" "!PROJECT_DIR!" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy template files.
    set "EXIT_RC=1"
    goto :FINISH
)

if not exist "!PROJECT_DIR!\.sparkle-project" (
    echo.> "!PROJECT_DIR!\.sparkle-project"
    echo [LOG] Created .sparkle-project marker.
)

echo [LOG] Configuring project files ^(replacing __PROJECT_NAME__^)...
powershell -NoProfile -Command ^
    "$projectRoot = '!PROJECT_DIR!'; " ^
    "$projectName = '!PROJECT_NAME!'; " ^
    "$extensions = @('.txt', '.cmake', '.cpp', '.h', '.hlsl', '.hlsli', '.json', '.md'); " ^
    "Get-ChildItem -Path $projectRoot -Recurse -File | Where-Object { $extensions -contains $_.Extension } | ForEach-Object { " ^
    "    $content = Get-Content -Path $_.FullName -Raw; " ^
    "    if ($content.Contains('__PROJECT_NAME__')) { " ^
    "        $updated = $content.Replace('__PROJECT_NAME__', $projectName); " ^
    "        Set-Content -Path $_.FullName -Value $updated -NoNewline; " ^
    "    } " ^
    "}"

if errorlevel 1 (
    echo [ERROR] Failed to configure project files.
    set "EXIT_RC=1"
    goto :FINISH
)

echo.
echo ============================================================
echo   [SUCCESS] Project Created
echo ============================================================
echo.
echo   Name:     !PROJECT_NAME!
echo   Location: !PROJECT_DIR!
echo.
echo   Next steps:
echo   1. Run GenerateSolution.bat to refresh the build files
echo   2. Open the generated solution through GenerateSolution.bat or Visual Studio
echo   3. Set !PROJECT_NAME!Editor as the startup target
echo   4. Use Build.bat !PROJECT_NAME! Both Debug to build both launch targets
echo.
echo ============================================================

echo.
echo ============================================================
echo   Refresh Build Files Now?
echo ============================================================
echo.
echo   Y^) Yes - Run GenerateSolution.bat now
echo   N^) No  - Skip for now
echo.
echo ============================================================

if "!INTERACTIVE!"=="0" goto :CONFIGURE_DONE

:CONFIGURE_PROMPT
set "DO_CONFIGURE="
set /P "DO_CONFIGURE=Enter choice [Y/N]: "
if /I "!DO_CONFIGURE!"=="Y" goto :RUN_CONFIGURE
if /I "!DO_CONFIGURE!"=="N" goto :CONFIGURE_DONE
if "!DO_CONFIGURE!"=="" goto :CONFIGURE_DONE
echo [WARN] Invalid input. Please enter Y or N.
goto :CONFIGURE_PROMPT

:RUN_CONFIGURE
echo.
echo [LOG] Refreshing build files...
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\GenerateSolution.bat" CONTINUE
set "CONFIGURE_RC=!ERRORLEVEL!"
set "PARENT_BATCH="
if "!CONFIGURE_RC!" NEQ "0" (
    echo [ERROR] GenerateSolution step failed after project creation.
    set "EXIT_RC=1"
    goto :FINISH
)

:CONFIGURE_DONE
set "EXIT_RC=0"
goto :FINISH

:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%"

if defined PARENT_BATCH (
    exit /B %EXIT_RC%
)

echo.
if "%EXIT_RC%"=="0" (
    echo ============================================================
    echo   [SUCCESS] Project creation completed.
    echo ============================================================
) else (
    echo ============================================================
    echo   [ERROR] Project creation failed.
    echo ============================================================
)
echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%