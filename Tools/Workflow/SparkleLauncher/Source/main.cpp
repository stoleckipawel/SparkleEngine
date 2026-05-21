#include "LauncherShell.h"

#include <iostream>

int main(int argc, char** argv)
{
	const SparkleLauncher::LauncherShell shell;
	return shell.Run(argc, argv, std::cout, std::cerr);
}