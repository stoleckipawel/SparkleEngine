#include "Cli/AssetConverterCommands.h"

#include <filesystem>
#include <iostream>
#include <string_view>

static void PrintUsage()
{
	std::cerr << "Usage:\n"
	          << "  AssetConverter cook-scene <source-scene-path>\n"
	          << "  AssetConverter collect-texture-requests <source-scene-path> <request-file-path>\n"
	          << "\n"
	          << "Compatibility:\n"
	          << "  AssetConverter <source-scene-path>\n";
}

int main(int argc, char** argv)
{
	if (argc == 2)
	{
		const std::string_view argument(argv[1]);
		if (argument != "cook-scene")
		{
			return AssetConverterCommands::RunCookScene(std::filesystem::path(argv[1]));
		}
	}

	if (argc == 3)
	{
		const std::string_view command(argv[1]);
		if (command == "cook-scene")
		{
			return AssetConverterCommands::RunCookScene(std::filesystem::path(argv[2]));
		}
	}

	if (argc == 4)
	{
		const std::string_view command(argv[1]);
		if (command == "collect-texture-requests")
		{
			return AssetConverterCommands::RunCollectTextureRequests(std::filesystem::path(argv[2]), std::filesystem::path(argv[3]));
		}

	}

	PrintUsage();
	return 1;
}
