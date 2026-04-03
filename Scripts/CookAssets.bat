@echo off
setlocal

if "%~1"=="" goto usage
if "%~2"=="" goto usage

set CONFIG=%~3
if "%CONFIG%"=="" set CONFIG=Debug

set EXE=%~dp0..\bin\%CONFIG%\SparkleAssetConverter.exe
if not exist "%EXE%" (
	echo SparkleAssetConverter not found at "%EXE%"
	echo Build target SparkleAssetConverter first.
	exit /b 1
)

"%EXE%" "%~1" "%~2"
exit /b %ERRORLEVEL%

:usage
echo Usage: CookAssets.bat ^<input_path^> ^<output_dir^> [Config]
exit /b 1