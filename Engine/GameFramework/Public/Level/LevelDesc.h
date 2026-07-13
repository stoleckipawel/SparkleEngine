#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"
#include "GameFramework/Public/Scene/Sky/SceneSkyDesc.h"

#include <optional>
#include <string>
#include <vector>

struct SPARKLE_ENGINE_API SceneAssetId
{
	std::string value;

	bool IsEmpty() const noexcept { return value.empty(); }
};

struct SPARKLE_ENGINE_API LevelDesc
{
	std::string name;
	CameraDesc cameraDesc;
	std::optional<SceneSkyDesc> sky;
	std::vector<SceneLightDesc> lights;
	std::vector<SceneAssetId> sceneAssetIds;
};
