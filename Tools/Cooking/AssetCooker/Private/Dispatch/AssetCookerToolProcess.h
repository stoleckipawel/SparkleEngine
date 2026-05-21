#pragma once

#include <filesystem>
#include <string>
#include <vector>

class AssetCookerToolProcess final
{
public:
	static int Run(
	    const std::filesystem::path& executablePath,
	    const std::vector<std::wstring>& arguments,
	    const std::filesystem::path& workingDirectory);
};
