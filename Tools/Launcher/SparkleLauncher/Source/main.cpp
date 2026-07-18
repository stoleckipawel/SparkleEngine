#include "LauncherGuiApp.h"
#include "../Private/Shell/LauncherShell.h"
#include "Core/Public/Threading/ThreadOwnership.h"

#include <iostream>
#include <string_view>

namespace SparkleLauncherMain
{
	bool ShouldRunShell(int argc, char** argv) noexcept
	{
		for (int index = 1; index < argc; ++index)
		{
			const std::string_view argument(argv[index]);
			if (argument == "--dry-run" || argument == "--run" || argument == "--help" || argument == "-h" || argument == "/?")
			{
				return true;
			}
		}
		return false;
	}
}

int main(int argc, char** argv)
{
	Threading::SetCurrentThreadRole("Sparkle.ToolMain");
#if !defined(_WIN32)
	std::cerr << "SparkleLauncher GUI is currently implemented for Windows.\n";
#endif
	if (SparkleLauncherMain::ShouldRunShell(argc, argv))
	{
		const SparkleLauncher::LauncherShell shell;
		return shell.Run(argc, argv, std::cout, std::cerr);
	}
	return SparkleLauncher::RunLauncherGui(argc, argv);
}
