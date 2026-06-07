#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Level/LevelDesc.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace Assets
{
	class SceneAssetPayloadLoader final
	{
	  public:
		static bool AppendSceneAsset(
		    const SceneAssetId& sceneAssetId,
		    const std::filesystem::path& manifestRelativePath,
		    SceneAssetPayload& sceneAssetPayload,
		    std::uint32_t& materialBaseIndex,
		    std::string& errorMessage);
	};
}
