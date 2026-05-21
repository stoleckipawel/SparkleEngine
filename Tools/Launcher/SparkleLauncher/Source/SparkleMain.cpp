#include "SparkleCli.h"

#include <iostream>

int main(int argc, char** argv)
{
	const SparkleLauncher::SparkleCli cli;
	return cli.Run(argc, argv, std::cout, std::cerr);
}