#pragma once

#include <filesystem>

class AssetCookerSourceInspection final
{
public:
	static int InspectSource(const std::filesystem::path& sourceScenePath);
	static int CollectTextureRequests(
	    const std::filesystem::path& sourceScenePath,
	    const std::filesystem::path& outputRequestPath);
};
