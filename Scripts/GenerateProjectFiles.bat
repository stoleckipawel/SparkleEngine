@echo off
:: ============================================================================
:: GenerateProjectFiles.bat - Visual Studio solution generator
:: ============================================================================
:: Generates the VS solution via CMake configure. Incremental by default —
:: preserves existing build artifacts and third-party dependencies.
::
::   1. Validates required build dependencies
::   2. Configures toolset (ClangCL preferred, MSVC fallback)
::   3. Generates Visual Studio solution via CMake
::
:: Usage: GenerateProjectFiles.bat [CONTINUE]
::   CONTINUE - Suppress interactive prompts (used by parent scripts)
::
:: Environment:
::   PARENT_BATCH  - When set, suppresses pause on completion
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
::
:: Exit Codes:
::   0 - Solution generated successfully
::   1 - Generation failed
:: ============================================================================

setlocal enabledelayedexpansion

:: ---------------------------------------------------------------------------
:: Check if running from parent script (CONTINUE argument)
:: ---------------------------------------------------------------------------
set "CALLED_FROM_PARENT=0"
if /I "%~1"=="CONTINUE" set "CALLED_FROM_PARENT=1"

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
:: Step 1: Validate build dependencies
:: ---------------------------------------------------------------------------
echo [LOG] Checking build dependencies...
set "PARENT_BATCH=1"
call "%~dp0CheckDependencies.bat" CONTINUE
if errorlevel 1 (
    set "PARENT_BATCH="
    echo [ERROR] Dependency check failed.
    set "EXIT_RC=1"
    goto :FINISH
)
set "PARENT_BATCH="

:: ---------------------------------------------------------------------------
:: Step 2: Configure build environment
:: ---------------------------------------------------------------------------
if "!USE_CLANG!"=="1" (
    echo [LOG] Toolset: ClangCL
) else (
    echo [LOG] Toolset: MSVC ^(Clang not found^)
)

:: ---------------------------------------------------------------------------
:: Step 3: Extract project name from CMakeLists.txt
:: ---------------------------------------------------------------------------
set "PROJECT_NAME="
for /f "tokens=2 delims=( " %%P in ('findstr /i "project(" "!ROOT_DIR!\CMakeLists.txt"') do (
    set "RAW_NAME=%%P"
)
:: Strip trailing parenthesis and whitespace
for /f "delims=) tokens=1" %%A in ("!RAW_NAME!") do set "PROJECT_NAME=%%A"
set "PROJECT_NAME=!PROJECT_NAME: =!"

echo [LOG] Project: !PROJECT_NAME!
set "SOLUTION_FILE=!BUILD_DIR!\!PROJECT_NAME!.sln"

:: ---------------------------------------------------------------------------
:: Step 4: Generate solution (incremental — only if needed)
:: ---------------------------------------------------------------------------
set "SOLUTION_GENERATED=0"
if not exist "!BUILD_DIR!\CMakeCache.txt" (
    set "SOLUTION_GENERATED=1"
    echo [LOG] No CMake cache found. Running full configure...
) else if not exist "!SOLUTION_FILE!" (
    set "SOLUTION_GENERATED=1"
    echo [LOG] Solution file missing. Regenerating...
) else (
    echo [LOG] CMake cache and solution exist. Running incremental configure...
    set "SOLUTION_GENERATED=1"
)

if "!SOLUTION_GENERATED!"=="1" (
    if not exist "!BUILD_DIR!" (
        echo [LOG] Creating build directory: !BUILD_DIR!
        mkdir "!BUILD_DIR!"
        if errorlevel 1 (
            echo [ERROR] Failed to create build directory.
            set "EXIT_RC=1"
            goto :FINISH
        )
    )

    pushd "!BUILD_DIR!"

    if "!USE_CLANG!"=="1" (
        echo [LOG] CMake: -G "!GENERATOR!" -A !ARCH! -T ClangCL
        cmake -G "!GENERATOR!" -A !ARCH! -T ClangCL "!ROOT_DIR!"
    ) else (
        echo [LOG] CMake: -G "!GENERATOR!" -A !ARCH!
        cmake -G "!GENERATOR!" -A !ARCH! "!ROOT_DIR!"
    )

    set "CMAKE_RC=!ERRORLEVEL!"
    popd

    if "!CMAKE_RC!" NEQ "0" (
        echo [ERROR] CMake generation failed.
        set "EXIT_RC=1"
        goto :FINISH
    )

    echo [LOG] Solution generated: !SOLUTION_FILE!
)

:: ---------------------------------------------------------------------------
:: Optional: Open solution after generation
:: ---------------------------------------------------------------------------
if "!CALLED_FROM_PARENT!"=="0" (
    if not defined PARENT_BATCH (
        echo.
        echo ============================================================
        echo   Open Visual Studio?
        echo ============================================================
        echo.
        echo   Y^) Yes - Open the generated solution
        echo   N^) No  - Continue without opening
        echo.
        echo ============================================================

        :OPEN_VS_PROMPT
        set "OPEN_VS="
        set /P "OPEN_VS=Enter choice [Y/N]: "

        if /I "!OPEN_VS!"=="Y" (
            echo.
            echo [LOG] Opening: !SOLUTION_FILE!
            start "" "!SOLUTION_FILE!"
            goto :AFTER_VS_PROMPT
        )
        if /I "!OPEN_VS!"=="N" goto :AFTER_VS_PROMPT
        if "!OPEN_VS!"=="" goto :AFTER_VS_PROMPT

        echo [WARN] Invalid input. Please enter Y or N.
        goto :OPEN_VS_PROMPT
    )
)
:AFTER_VS_PROMPT

echo.
echo [LOG] GenerateProjectFiles.bat completed.
set "EXIT_RC=0"
goto :FINISH

:: ============================================================================
:: Clean exit with proper endlocal handling
:: ============================================================================
:FINISH
set "_TMP_LOGFILE=%LOGFILE%"
set "_TMP_CALLED_FROM_PARENT=%CALLED_FROM_PARENT%"
set "_TMP_RC=%EXIT_RC%"
endlocal & set "LOGFILE=%_TMP_LOGFILE%" & set "EXIT_RC=%_TMP_RC%" & set "CALLED_FROM_PARENT=%_TMP_CALLED_FROM_PARENT%"

if defined PARENT_BATCH (
    exit /B %EXIT_RC%
)

if "%CALLED_FROM_PARENT%"=="1" (
    exit /B %EXIT_RC%
)

echo.
if "%EXIT_RC%"=="0" (
    echo.
    echo ============================================================
    echo   [SUCCESS] Solution generation completed.
    echo ============================================================
) else (
    echo.
    echo ============================================================
    echo   [ERROR] Solution generation failed.
    echo ============================================================
)
echo.
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%

