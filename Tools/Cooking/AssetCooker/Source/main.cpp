#include "../Private/Cli/AssetCookerCli.h"

#include "Core/Public/Threading/ThreadOwnership.h"

int main(int argc, char** argv)
{
	Threading::SetCurrentThreadRole("Sparkle.ToolMain");
	const AssetCookerCli cli;
	return cli.Run(argc, argv);
}
