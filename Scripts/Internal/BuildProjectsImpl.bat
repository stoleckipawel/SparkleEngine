@echo off
:: ============================================================================
:: BuildProjectsImpl.bat - Internal project build implementation
:: ============================================================================
:: Builds projects under projects/ for a specified configuration.
:: Can build all projects or a specific project.
::
:: Usage: BuildProjectsImpl.bat <Configuration> [ProjectName]
::   Configuration: Debug | Release | RelWithDebInfo
::   ProjectName: Optional - specific project name or "ALL" for all projects
::
:: Environment:
::   PARENT_BATCH  - When set, suppresses pause on completion
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
:: ============================================================================

setlocal enabledelayedexpansion

:: ---------------------------------------------------------------------------
:: Logging bootstrap (if not already captured by parent)
:: ---------------------------------------------------------------------------
if not defined LOG_CAPTURED (
    call "%~dp0BootstrapLog.bat" "%~f0" %*
    exit /B !ERRORLEVEL!
)

:: ---------------------------------------------------------------------------
:: Load shared configuration
:: ---------------------------------------------------------------------------
call "%~dp0Config.bat"

:: Initialize exit code (default to failure — set to 0 on success)
set "EXIT_RC=1"

:: ---------------------------------------------------------------------------
:: Parse arguments
:: ---------------------------------------------------------------------------
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

set "TARGET_PROJECT=%~2"
if "%TARGET_PROJECT%"=="" set "TARGET_PROJECT=ALL"

:: MSBuild properties for RelWithDebInfo: preserve full debug symbols
set "MSBUILD_EXTRA="
if /I "%CONFIG%"=="RelWithDebInfo" (
    echo [LOG] RelWithDebInfo: Enabling full debug information.
    set "MSBUILD_EXTRA=/p:DebugSymbols=true /p:DebugType=full /p:GenerateDebugInformation=true /p:Optimize=true /p:LinkIncremental=false"
)

:: ---------------------------------------------------------------------------
:: Ensure solution exists (generate if missing)
:: ---------------------------------------------------------------------------
:: PROJECT_NAME and SOLUTION_FILE are provided by Config.bat.
if not exist "!SOLUTION_FILE!" (
    echo [LOG] Solution not found. Invoking GenerateProjectFiles.bat...
    set "PARENT_BATCH=1"
    call "!SCRIPTS_DIR!\GenerateProjectFiles.bat" CONTINUE
    set "PARENT_BATCH="
    if errorlevel 1 (
        echo [ERROR] Solution generation failed. Cannot build.
        echo         Run Scripts\GenerateProjectFiles.bat standalone for details.
        set "EXIT_RC=1"
        goto :FINISH
    )
)

:: ---------------------------------------------------------------------------
:: Build project(s)
:: ---------------------------------------------------------------------------
set "BUILD_FAILED=0"
set "BUILD_COUNT=0"

if /I "!TARGET_PROJECT!"=="ALL" (
    :: Build all projects
    for /D %%S in ("!PROJECTS_DIR!\*") do (
        set "PROJ_NAME=%%~nxS"
        :: Skip TemplateProject
        if /I "!PROJ_NAME!" NEQ "TemplateProject" (
            set "PROJECT_VCXPROJ=!BUILD_DIR!\Projects\!PROJ_NAME!\!PROJ_NAME!.vcxproj"
            
            if exist "!PROJECT_VCXPROJ!" (
                set /A "BUILD_COUNT+=1"
                echo [LOG] Building: !PROJ_NAME! [!CONFIG!]
                msbuild "!PROJECT_VCXPROJ!" /p:Configuration=!CONFIG! /p:Platform=!ARCH! !MSBUILD_EXTRA! /nologo /v:minimal
                if errorlevel 1 (
                    echo [ERROR] Build failed: !PROJ_NAME! [!CONFIG!]
                    set "BUILD_FAILED=1"
                ) else (
                    echo [SUCCESS] !PROJ_NAME! [!CONFIG!] - Output: bin\!CONFIG!
                )
            )
        )
    )
) else (
    :: Build specific project
    set "PROJECT_VCXPROJ=!BUILD_DIR!\Projects\!TARGET_PROJECT!\!TARGET_PROJECT!.vcxproj"
    
    if exist "!PROJECT_VCXPROJ!" (
        set /A "BUILD_COUNT+=1"
        echo [LOG] Building: !TARGET_PROJECT! [!CONFIG!]
        msbuild "!PROJECT_VCXPROJ!" /p:Configuration=!CONFIG! /p:Platform=!ARCH! !MSBUILD_EXTRA! /nologo /v:minimal
        if errorlevel 1 (
            echo [ERROR] Build failed: !TARGET_PROJECT! [!CONFIG!]
            set "BUILD_FAILED=1"
        ) else (
            echo [SUCCESS] !TARGET_PROJECT! [!CONFIG!] - Output: bin\!CONFIG!
        )
    ) else (
        echo [ERROR] Project not found: !TARGET_PROJECT!
        echo         Expected: !PROJECT_VCXPROJ!
        set "BUILD_FAILED=1"
    )
)

if "!BUILD_COUNT!"=="0" (
    echo [WARN] No projects were built. Ensure solution is generated.
)

if "!BUILD_FAILED!"=="1" (
    set "EXIT_RC=1"
    goto :FINISH
)

if /I "!TARGET_PROJECT!"=="ALL" (
    echo [LOG] All projects built successfully for !CONFIG!.
) else (
    echo [LOG] !TARGET_PROJECT! built successfully for !CONFIG!.
)
set "EXIT_RC=0"
goto :FINISH

:: ---------------------------------------------------------------------------
:: Clean exit with proper endlocal handling
:: ---------------------------------------------------------------------------
:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_RC=%EXIT_RC%"
set "_TMP_PARENT=%PARENT_BATCH%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%" & set "PARENT_BATCH=%_TMP_PARENT%"

if defined PARENT_BATCH (
    exit /B %EXIT_RC%
)

echo.
echo ============================================================
if "%EXIT_RC%"=="0" (
    echo   [SUCCESS] Project build completed.
) else (
    echo   [ERROR] Project build failed.
)
echo ============================================================
echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%
