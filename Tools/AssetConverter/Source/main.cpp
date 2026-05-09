#include "Cli/AssetConverterCommands.h"

#include <charconv>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <system_error>

static bool TryParseNonNegativeInt(std::string_view value, int& outValue) noexcept
{
	int parsedValue = 0;
	const std::from_chars_result result = std::from_chars(value.data(), value.data() + value.size(), parsedValue);
	if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || parsedValue < 0)
	{
		return false;
	}

	outValue = parsedValue;
	return true;
}

static void PrintUsage()
{
	std::cerr << "Usage:\n"
	          << "  AssetConverter cook-scene <source-scene-path>\n"
	          << "  AssetConverter collect-texture-requests <source-scene-path> <request-file-path>\n"
	          << "  AssetConverter cook-scene-list <scene-list-file> <total-scene-count>\n"
	          << "  AssetConverter collect-texture-request-list <scene-list-file> <total-scene-count> <request-file-path>\n"
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

		if (command == "cook-scene-list")
		{
			int totalSceneCount = 0;
			if (!TryParseNonNegativeInt(argv[3], totalSceneCount))
			{
				std::cerr << "AssetConverter: total scene count must be a non-negative integer.\n";
				return 1;
			}

			return AssetConverterCommands::RunCookSceneList(std::filesystem::path(argv[2]), totalSceneCount);
		}
	}

	if (argc == 5)
	{
		const std::string_view command(argv[1]);
		int totalSceneCount = 0;
		if (!TryParseNonNegativeInt(argv[3], totalSceneCount))
		{
			std::cerr << "AssetConverter: total scene count must be a non-negative integer.\n";
			return 1;
		}

		if (command == "collect-texture-request-list")
		{
			return AssetConverterCommands::RunCollectTextureRequestList(
			    std::filesystem::path(argv[2]),
			    std::filesystem::path(argv[4]),
			    totalSceneCount);
		}
	}

	PrintUsage();
	return 1;
}
