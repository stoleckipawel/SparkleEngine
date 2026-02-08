@echo off
:: ============================================================================
:: RunClangFormat.bat - Code formatting utility
:: ============================================================================
:: Runs clang-format on all source files under Engine/ and Projects/.
:: Displays progress, tracks modifications, and generates a summary log.
::
:: Supported extensions: .cpp, .h, .hlsl, .hlsli
:: Output log: logs/LogClangFormat.txt
::
:: Usage: RunClangFormat.bat
::
:: Environment:
::   PARENT_BATCH  - When set, suppresses pause on completion
::   LOG_CAPTURED  - Indicates logging is already active
::   LOGFILE       - Path to current log file
::
:: Exit Codes:
::   0 - Formatting completed successfully
::   1 - clang-format not found or errors occurred
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
:: Validate clang-format is available
:: ---------------------------------------------------------------------------
where clang-format >nul 2>&1
if errorlevel 1 (
    echo [ERROR] clang-format not found. Install clang-format and add to PATH.
    set "EXIT_RC=1"
    goto :FINISH
)

:: ---------------------------------------------------------------------------
:: Prepare format-specific log file
:: ---------------------------------------------------------------------------
set "LOG_DIR=!ROOT_DIR!\logs"
if not exist "!LOG_DIR!" mkdir "!LOG_DIR!"
set "FORMAT_LOG=!LOG_DIR!\LogClangFormat.txt"

echo [LOG] ClangFormat started: %DATE% %TIME% > "!FORMAT_LOG!"

:: ---------------------------------------------------------------------------
:: Count total files to process (for progress display)
:: ---------------------------------------------------------------------------
echo [LOG] Scanning for source files...

set /A TOTAL=0
for %%E in (cpp h hlsl hlsli) do (
    for /R "!ENGINE_DIR!" %%F in (*.%%E) do (
        echo %%~fF | findstr /I /C:"\third_party\" >nul || set /A TOTAL+=1
    )
    if exist "!PROJECTS_DIR!" (
        for /R "!PROJECTS_DIR!" %%F in (*.%%E) do (
            set /A TOTAL+=1
        )
    )
)

if %TOTAL%==0 (
    echo [WARN] No source files found.
    echo [LOG] No files found. >> "!FORMAT_LOG!"
    goto :SUMMARY
)

echo [LOG] Found %TOTAL% files to process.
echo.

:: ---------------------------------------------------------------------------
:: Process each file
:: ---------------------------------------------------------------------------
set /A IDX=0
set /A MODIFIED=0
set /A FORMAT_ERRORS=0

for %%E in (cpp h hlsl hlsli) do (
    call :PROCESS_DIR "!ENGINE_DIR!" %%E
    if exist "!PROJECTS_DIR!" call :PROCESS_DIR "!PROJECTS_DIR!" %%E
)

goto :SUMMARY

:: ---------------------------------------------------------------------------
:: Subroutine: Process all files of given extension in directory tree
:: ---------------------------------------------------------------------------
:PROCESS_DIR
set "SCAN_DIR=%~1"
set "EXT=%~2"

for /R "%SCAN_DIR%" %%F in (*.%EXT%) do (
    :: Skip files in third_party directory
    echo %%~fF | findstr /I /C:"\third_party\" >nul
    if errorlevel 1 (
        set /A IDX+=1
        set "FILE=%%~fF"

        :: Display progress
        echo [!IDX!/!TOTAL!] !FILE!
        echo [SCAN] !IDX!/!TOTAL! - !FILE! >> "!FORMAT_LOG!"

        :: Compute hash before formatting
        set "HASH_BEFORE="
        for /f "skip=1 tokens=1" %%H in ('certutil -hashfile "%%~fF" MD5 2^>nul') do (
            if not defined HASH_BEFORE set "HASH_BEFORE=%%H"
        )

        :: Run clang-format in-place
        clang-format -style=file -i "%%~fF" 2>>"!FORMAT_LOG!"
        if errorlevel 1 (
            echo [ERROR] clang-format failed: %%~fF >> "!FORMAT_LOG!"
            set /A FORMAT_ERRORS+=1
        )

        :: Compute hash after formatting
        set "HASH_AFTER="
        for /f "skip=1 tokens=1" %%H in ('certutil -hashfile "%%~fF" MD5 2^>nul') do (
            if not defined HASH_AFTER set "HASH_AFTER=%%H"
        )

        :: Check if file was modified
        if not "!HASH_BEFORE!"=="!HASH_AFTER!" (
            set /A MODIFIED+=1
            echo [MODIFIED] !FILE! >> "!FORMAT_LOG!"
            echo   ^> Modified
        )
    )
)
goto :EOF

:: ---------------------------------------------------------------------------
:: Summary
:: ---------------------------------------------------------------------------
:SUMMARY
echo.
echo ============================================================
echo   ClangFormat Summary
echo ============================================================
echo.
echo   Scanned:  !IDX! files
echo   Modified: !MODIFIED! files
if "!FORMAT_ERRORS!" NEQ "0" (
    echo   Errors:   !FORMAT_ERRORS! files
)
echo.
echo ============================================================

echo [LOG] Summary: Scanned=!IDX! Modified=!MODIFIED! Errors=!FORMAT_ERRORS! >> "!FORMAT_LOG!"
echo [LOG] ClangFormat finished: %DATE% %TIME% >> "!FORMAT_LOG!"

if "!FORMAT_ERRORS!" NEQ "0" (
    set "EXIT_RC=1"
) else (
    set "EXIT_RC=0"
)
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
echo [LOG] Logs: %LOGFILE%
pause
exit /B %EXIT_RC%

