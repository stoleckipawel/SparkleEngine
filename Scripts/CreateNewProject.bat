@echo off
:: ============================================================================
:: CreateNewProject.bat - Sparkle Project Generator
:: ============================================================================
:: Creates a new Sparkle project from the TemplateProject.
::
:: Usage: CreateNewProject.bat [ProjectName]
::   If ProjectName is omitted, prompts interactively.
::
:: The script will:
::   1. Copy the template from Projects/TemplateProject
::   2. Replace __PROJECT_NAME__ placeholder with actual project name
::   3. Create .sparkle-project marker file
::   4. Prompt to regenerate the VS solution
::
:: Environment:
::   PARENT_BATCH  - When set, suppresses pause on completion
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
::
:: Exit Codes:
::   0 - Project created successfully
::   1 - Creation failed
:: ============================================================================

setlocal enabledelayedexpansion

:: ---------------------------------------------------------------------------
:: Logging bootstrap
:: ---------------------------------------------------------------------------
if not defined LOG_CAPTURED (
    call "%~dp0Internal\BootstrapLog.bat" "%~f0" %*
    exit /B %ERRORLEVEL%
)

:: ---------------------------------------------------------------------------
:: Load shared configuration
:: ---------------------------------------------------------------------------
call "%~dp0Internal\Config.bat"

:: ---------------------------------------------------------------------------
:: Resolve template path
:: ---------------------------------------------------------------------------
set "TEMPLATE_DIR=!PROJECTS_DIR!\TemplateProject"

:: ---------------------------------------------------------------------------
:: Validate template exists
:: ---------------------------------------------------------------------------
if not exist "!TEMPLATE_DIR!" (
    echo [ERROR] Template directory not found: !TEMPLATE_DIR!
    echo         Please ensure the Projects\TemplateProject folder exists.
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Get project name from argument or prompt
:: ---------------------------------------------------------------------------
set "PROJECT_NAME=%~1"
if "!PROJECT_NAME!"=="" (
    echo.
    echo ============================================================
    echo   Sparkle Project Generator
    echo ============================================================
    echo.
    set /p "PROJECT_NAME=Enter project name (e.g. MyGame): "
)

:: Validate project name is not empty
if "!PROJECT_NAME!"=="" (
    echo [ERROR] Project name cannot be empty.
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Validate project name (alphanumeric + underscore only)
:: ---------------------------------------------------------------------------
echo !PROJECT_NAME!| findstr /r "^[A-Za-z_][A-Za-z0-9_]*$" >nul
if errorlevel 1 (
    echo [ERROR] Invalid project name: !PROJECT_NAME!
    echo         Project name must start with a letter or underscore,
    echo         and contain only letters, numbers, and underscores.
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Check if project already exists
:: ---------------------------------------------------------------------------
set "PROJECT_DIR=!PROJECTS_DIR!\!PROJECT_NAME!"
if exist "!PROJECT_DIR!" (
    echo [ERROR] Project already exists: !PROJECT_DIR!
    echo         Choose a different name or delete the existing project.
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Create project directory structure
:: ---------------------------------------------------------------------------
echo.
echo [LOG] Creating project: !PROJECT_NAME!
echo [LOG] Destination: !PROJECT_DIR!
echo.

:: Create projects folder if it doesn't exist
if not exist "!PROJECTS_DIR!" (
    echo [LOG] Creating projects directory...
    mkdir "!PROJECTS_DIR!"
)

:: ---------------------------------------------------------------------------
:: Copy template to new project
:: ---------------------------------------------------------------------------
echo [LOG] Copying template files...
xcopy /E /I /Q "!TEMPLATE_DIR!" "!PROJECT_DIR!" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy template files.
    set "EXIT_RC=1"
    goto :FINISH
)

:: Ensure the project marker exists (template should have one, but guarantee it)
if not exist "!PROJECT_DIR!\.sparkle-project" (
    echo.> "!PROJECT_DIR!\.sparkle-project"
    echo [LOG] Created .sparkle-project marker.
)

:: ---------------------------------------------------------------------------
:: Replace __PROJECT_NAME__ placeholder in all project files
:: ---------------------------------------------------------------------------
:: Recursively process all text files so adding new template files
:: with placeholders "just works" without updating this script.
echo [LOG] Configuring project files ^(replacing __PROJECT_NAME__^)...

for /R "!PROJECT_DIR!" %%F in (*.txt *.cmake *.cpp *.h *.hlsl *.hlsli *.json *.md) do (
    findstr /M /C:"__PROJECT_NAME__" "%%F" >nul 2>&1
    if not errorlevel 1 (
        call :REPLACE_PLACEHOLDER "%%F"
    )
)

:: ---------------------------------------------------------------------------
:: Success message
:: ---------------------------------------------------------------------------
echo.
echo ============================================================
echo   [SUCCESS] Project Created
echo ============================================================
echo.
echo   Name:     !PROJECT_NAME!
echo   Location: !PROJECT_DIR!
echo.
echo   Next steps:
echo   1. Run GenerateProjectFiles.bat to regenerate VS solution
echo   2. Open build\Sparkle.sln in Visual Studio
echo   3. Set !PROJECT_NAME! as startup project
echo.
echo ============================================================

:: Ask to rebuild solution
echo.
echo ============================================================
echo   Regenerate VS Solution?
echo ============================================================
echo.
echo   Y^) Yes - Regenerate now
echo   N^) No  - Skip for now
echo.
echo ============================================================

:REBUILD_PROMPT
set "REBUILD="
set /p "REBUILD=Enter choice [Y/N]: "
if /i "!REBUILD!"=="Y" goto :DO_REBUILD
if /i "!REBUILD!"=="N" goto :SKIP_REBUILD
if "!REBUILD!"=="" goto :SKIP_REBUILD
echo [WARN] Invalid input. Please enter Y or N.
goto :REBUILD_PROMPT

:DO_REBUILD
echo.
echo [LOG] Regenerating Visual Studio solution...
set "PARENT_BATCH=1"
call "!SCRIPTS_DIR!\GenerateProjectFiles.bat" CONTINUE
set "PARENT_BATCH="

:SKIP_REBUILD
set "EXIT_RC=0"
goto :FINISH

:: ============================================================================
:: Clean exit with proper endlocal handling
:: ============================================================================
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

:: ---------------------------------------------------------------------------
:: Subroutine: Replace __PROJECT_NAME__ in a file using PowerShell
:: ---------------------------------------------------------------------------
:REPLACE_PLACEHOLDER
set "TARGET_FILE=%~1"
if not exist "%TARGET_FILE%" (
    echo [WARN] File not found for substitution: %TARGET_FILE%
    exit /B 0
)

:: Use PowerShell for reliable text replacement
powershell -NoProfile -Command ^
    "$content = Get-Content -Path '%TARGET_FILE%' -Raw; " ^
    "$content = $content -replace '__PROJECT_NAME__', '%PROJECT_NAME%'; " ^
    "Set-Content -Path '%TARGET_FILE%' -Value $content -NoNewline"

if errorlevel 1 (
    echo [WARN] Failed to process: %TARGET_FILE%
)
exit /B 0
