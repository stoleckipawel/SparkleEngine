#pragma once

#include <filesystem>
#include <stop_token>
#include <string>
#include <vector>

class AssetCookerToolProcess final
{
public:
	static int Run(
	    const std::filesystem::path& executablePath,
	    const std::vector<std::string>& arguments,
	    const std::filesystem::path& workingDirectory,
	    std::stop_token cancellation = {});
};
