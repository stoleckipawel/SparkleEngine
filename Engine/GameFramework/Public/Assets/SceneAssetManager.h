#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Assets/SceneAssetPayload.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace Assets
{
	class SceneAssetRegistry;

	struct SPARKLE_ENGINE_API SceneAssetLoadResult
	{
		SceneAssetPayload sceneAssetPayload;
		std::string errorMessage;

		bool Succeeded() const noexcept { return errorMessage.empty(); }
	};

	class SPARKLE_ENGINE_API SceneAssetManager final
	{
	  public:
		SceneAssetManager();
		~SceneAssetManager() noexcept;

		SceneAssetManager(const SceneAssetManager&) = delete;
		SceneAssetManager& operator=(const SceneAssetManager&) = delete;
		SceneAssetManager(SceneAssetManager&&) = delete;
		SceneAssetManager& operator=(SceneAssetManager&&) = delete;

		SceneAssetLoadResult LoadSceneAsset(const SceneAssetId& sceneAssetId);
		SceneAssetLoadResult LoadSceneAssets(std::span<const SceneAssetId> sceneAssetIds);
		void UnloadAll() noexcept;

	  private:
		bool EnsureRegistryLoaded(std::string& errorMessage);
		bool AppendSceneAssetToPayload(
		    const SceneAssetId& sceneAssetId,
		    SceneAssetPayload& sceneAssetPayload,
		    std::uint32_t& materialBaseIndex,
		    std::string& errorMessage);

		std::unique_ptr<SceneAssetRegistry> m_sceneAssetRegistry;
		bool m_sceneAssetRegistryLoaded = false;
		std::vector<std::string> m_loadedSceneAssetIds;
	};
}
