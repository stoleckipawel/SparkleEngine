#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Lighting/LevelLightingDesc.h"

#include <filesystem>
#include <string>
#include <vector>

struct SPARKLE_ENGINE_API ImportedMeshRequest
{
	std::filesystem::path assetPath;
};

struct SPARKLE_ENGINE_API LevelDesc
{
	std::string name;
	CameraDesc cameraDesc;
	LevelLightingDesc lightingDesc;
	std::vector<ImportedMeshRequest> importedMeshRequests;
};
