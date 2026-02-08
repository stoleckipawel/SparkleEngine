@echo off
:: ============================================================================
:: BuildSolution.bat - DEPRECATED wrapper
:: ============================================================================
:: This script has been replaced by GenerateProjectFiles.bat.
:: It is kept for backward compatibility and will be removed in a future update.
::
:: Usage: BuildSolution.bat [CONTINUE]
:: ============================================================================

echo [WARN] BuildSolution.bat is deprecated. Use GenerateProjectFiles.bat instead.
call "%~dp0GenerateProjectFiles.bat" %*
exit /B %ERRORLEVEL%
