#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <filesystem>
#include <vector>

struct SPARKLE_ENGINE_API ImportedMeshRequest
{
	std::filesystem::path assetPath;
};

struct SPARKLE_ENGINE_API LevelDesc
{
	std::vector<ImportedMeshRequest> importedMeshRequests;
};
