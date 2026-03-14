#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include "Core/Public/Math/Transform.h"

#include <filesystem>
#include <vector>

struct SPARKLE_ENGINE_API ImportedMeshRequest
{
	std::filesystem::path assetPath;
};

struct SPARKLE_ENGINE_API LevelCameraDesc
{
	Transform transform{{0.0f, 0.0f, -4.0f}};
};

struct SPARKLE_ENGINE_API LevelDesc
{
	LevelCameraDesc initialCamera;
	std::vector<ImportedMeshRequest> importedMeshRequests;
};
