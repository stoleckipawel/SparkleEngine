#include "LauncherGuiApp.h"

#include <iostream>

int main(int argc, char** argv)
{
#if !defined(_WIN32)
	std::cerr << "SparkleLauncher GUI is currently implemented for Windows.\n";
#endif
	return SparkleLauncher::RunLauncherGui(argc, argv);
}
