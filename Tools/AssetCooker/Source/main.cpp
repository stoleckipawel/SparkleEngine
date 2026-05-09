#include "../Private/Cli/AssetCookerCli.h"

int main(int argc, char** argv)
{
	const AssetCookerCli cli;
	return cli.Run(argc, argv);
}
