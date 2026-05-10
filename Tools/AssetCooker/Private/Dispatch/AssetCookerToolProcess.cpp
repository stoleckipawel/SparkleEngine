#include "AssetCookerToolProcess.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/ScopedLogEvent.h"
#include "Core/Public/Diagnostics/Trace.h"

#include <cwctype>
#include <iostream>

#if defined(_WIN32)
	#define NOMINMAX
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
#else
	#include <cstdlib>
#endif

static std::wstring AssetCookerQuoteCommandArgument(const std::wstring& argument)
{
	if (argument.empty())
	{
		return L"\"\"";
	}

	bool needsQuotes = false;
	for (const wchar_t character : argument)
	{
		if (std::iswspace(character) || character == L'\"')
		{
			needsQuotes = true;
			break;
		}
	}

	if (!needsQuotes)
	{
		return argument;
	}

	std::wstring quotedArgument;
	quotedArgument.push_back(L'\"');
	int backslashCount = 0;
	for (const wchar_t character : argument)
	{
		if (character == L'\\')
		{
			++backslashCount;
			continue;
		}

		if (character == L'\"')
		{
			quotedArgument.append(static_cast<std::size_t>(backslashCount * 2 + 1), L'\\');
			quotedArgument.push_back(character);
			backslashCount = 0;
			continue;
		}

		quotedArgument.append(static_cast<std::size_t>(backslashCount), L'\\');
		backslashCount = 0;
		quotedArgument.push_back(character);
	}

	quotedArgument.append(static_cast<std::size_t>(backslashCount * 2), L'\\');
	quotedArgument.push_back(L'\"');
	return quotedArgument;
}

static std::wstring AssetCookerBuildCommandLine(
    const std::filesystem::path& executablePath,
    const std::vector<std::wstring>& arguments)
{
	std::wstring commandLine = AssetCookerQuoteCommandArgument(executablePath.wstring());
	for (const std::wstring& argument : arguments)
	{
		commandLine.push_back(L' ');
		commandLine += AssetCookerQuoteCommandArgument(argument);
	}
	return commandLine;
}

int AssetCookerToolProcess::Run(
    const std::filesystem::path& executablePath,
    const std::vector<std::wstring>& arguments,
    const std::filesystem::path& workingDirectory)
{
	static const auto toolProcessLogger = Logging::GetOrCreateLogger("Tools.AssetCooker.Process");
	const std::string processScopeName = "AssetCooker.ToolProcess." + executablePath.filename().string();
	SPARKLE_CPU_SCOPE(processScopeName);
	SPARKLE_LOG_SCOPE(toolProcessLogger, spdlog::level::info, processScopeName);

	std::wcout << L"[LOG] Running: " << executablePath.wstring();
	for (const std::wstring& argument : arguments)
	{
		std::wcout << L" " << AssetCookerQuoteCommandArgument(argument);
	}
	std::wcout << L"\n";

#if defined(_WIN32)
	std::wstring commandLine = AssetCookerBuildCommandLine(executablePath, arguments);
	STARTUPINFOW startupInfo = {};
	startupInfo.cb = sizeof(startupInfo);
	PROCESS_INFORMATION processInformation = {};

	const std::wstring workingDirectoryString = workingDirectory.wstring();
	const BOOL createdProcess = CreateProcessW(
	    nullptr,
	    commandLine.data(),
	    nullptr,
	    nullptr,
	    TRUE,
	    0,
	    nullptr,
	    workingDirectoryString.c_str(),
	    &startupInfo,
	    &processInformation);

	if (!createdProcess)
	{
		std::wcerr << L"AssetCooker: failed to launch '" << executablePath.wstring() << L"'.\n";
		SPDLOG_LOGGER_ERROR(toolProcessLogger, "Failed to launch process '{}'", executablePath.string());
		return 1;
	}

	WaitForSingleObject(processInformation.hProcess, INFINITE);
	DWORD exitCode = 1;
	GetExitCodeProcess(processInformation.hProcess, &exitCode);
	CloseHandle(processInformation.hThread);
	CloseHandle(processInformation.hProcess);
	SPDLOG_LOGGER_INFO(toolProcessLogger, "Process '{}' exited with code {}", executablePath.string(), exitCode);
	return static_cast<int>(exitCode);
#else
	const std::wstring commandLine = AssetCookerBuildCommandLine(executablePath, arguments);
	const std::string narrowCommandLine(commandLine.begin(), commandLine.end());
	const int exitCode = std::system(narrowCommandLine.c_str());
	SPDLOG_LOGGER_INFO(toolProcessLogger, "Process '{}' exited with code {}", executablePath.string(), exitCode);
	return exitCode;
#endif
}
