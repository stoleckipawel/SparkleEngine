#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Settings/EditorRestartService.h"

#include "Window/Window.h"

#include <shellapi.h>

#include <string>
#include <vector>

class EditorRelaunchCommandBuilder final
{
  public:
	static std::wstring QuoteCommandLineArgument(const std::wstring& argument)
	{
		const bool needsQuotes = argument.find_first_of(L" \t\"") != std::wstring::npos;
		if (!needsQuotes)
		{
			return argument;
		}

		std::wstring result;
		result.reserve(argument.size() + 2);
		result += L'"';
		for (const wchar_t character : argument)
		{
			if (character == L'"')
			{
				result += L'\\';
			}
			result += character;
		}
		result += L'"';
		return result;
	}

	static std::wstring BuildRelaunchCommandLine(const std::filesystem::path& executablePath)
	{
		int argumentCount = 0;
		LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
		std::wstring result = QuoteCommandLineArgument(executablePath.wstring());
		if (arguments == nullptr)
		{
			return result;
		}

		for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
		{
			result += L' ';
			result += QuoteCommandLineArgument(arguments[argumentIndex]);
		}

		LocalFree(arguments);
		return result;
	}
};

bool EditorRestartService::Restart(Window& hostWindow) const
{
	const std::filesystem::path executablePath = Filesystem::GetExecutablePath();
	const std::wstring commandLine = EditorRelaunchCommandBuilder::BuildRelaunchCommandLine(executablePath);
	std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
	mutableCommandLine.push_back(L'\0');

	const std::wstring workingDirectory = Filesystem::GetWorkingDirectory().wstring();
	STARTUPINFOW startupInfo{};
	startupInfo.cb = sizeof(startupInfo);
	PROCESS_INFORMATION processInfo{};
	const BOOL launched = CreateProcessW(
	    executablePath.c_str(),
	    mutableCommandLine.data(),
	    nullptr,
	    nullptr,
	    FALSE,
	    0,
	    nullptr,
	    workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
	    &startupInfo,
	    &processInfo);
	if (launched == FALSE)
	{
		return false;
	}

	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);

	hostWindow.RequestClose();
	return true;
}
