#include "LauncherGuiApp.h"

#if defined(_WIN32)
	#define NOMINMAX
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>

	int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
	{
		return SparkleLauncher::RunLauncherGui();
	}
#else
	#include <iostream>

	int main()
	{
		std::cerr << "SparkleLauncher GUI is currently implemented for Windows. Use Sparkle.exe for CLI automation.\n";
		return SparkleLauncher::RunLauncherGui();
	}
#endif