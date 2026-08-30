#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"
#include "GameFramework/Public/Scene/Sky/SceneSkyDesc.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct SPARKLE_ENGINE_API SceneAssetId
{
	std::string value;
	std::string catalogValue;

	bool IsEmpty() const noexcept { return value.empty(); }
	std::string_view GetCatalogValue() const noexcept
	{
		return catalogValue.empty() ? std::string_view(value) : std::string_view(catalogValue);
	}
};

struct SPARKLE_ENGINE_API LevelDesc
{
	std::string name;
	CameraDesc cameraDesc;
	std::optional<SceneSkyDesc> sky;
	std::vector<SceneLightDesc> lights;
	std::vector<SceneAssetId> sceneAssetIds;
};
