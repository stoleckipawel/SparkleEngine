#include "PCH.h"

#include "Settings/EditorRestartService.h"

#include "Core/Public/Paths/DirectoryPaths.h"
#include "Window/Window.h"

#include <shellapi.h>

#include <string>

namespace
{
	std::wstring BuildRelaunchArguments()
	{
		int argumentCount = 0;
		LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
		if (arguments == nullptr || argumentCount <= 1)
		{
			if (arguments != nullptr)
			{
				LocalFree(arguments);
			}
			return {};
		}

		std::wstring result;
		for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
		{
			if (!result.empty())
			{
				result += L' ';
			}

			const std::wstring argument = arguments[argumentIndex];
			const bool needsQuotes = argument.find_first_of(L" \t\"") != std::wstring::npos;
			if (!needsQuotes)
			{
				result += argument;
				continue;
			}

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
		}

		LocalFree(arguments);
		return result;
	}
}

bool EditorRestartService::Restart(Window& hostWindow) const
{
	const std::filesystem::path executablePath = Paths::ExecutablePath();
	const std::wstring arguments = BuildRelaunchArguments();
	const HINSTANCE launched = ShellExecuteW(
	    nullptr,
	    L"open",
	    executablePath.c_str(),
	    arguments.empty() ? nullptr : arguments.c_str(),
	    executablePath.parent_path().c_str(),
	    SW_SHOWNORMAL);
	if (reinterpret_cast<std::intptr_t>(launched) <= 32)
	{
		return false;
	}

	hostWindow.RequestClose();
	return true;
}
