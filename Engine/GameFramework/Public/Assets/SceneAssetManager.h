#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"
#include "GameFramework/Public/Scene/RuntimeScenePayload.h"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace Engine::Assets
{
	struct SPARKLE_ENGINE_API SceneAssetLoadResult
	{
		RuntimeScenePayload payload;
		std::string errorMessage;

		bool Succeeded() const noexcept { return errorMessage.empty(); }
	};

	class SPARKLE_ENGINE_API SceneAssetManager final
	{
	  public:
		SceneAssetManager() noexcept = default;
		~SceneAssetManager() noexcept = default;

		SceneAssetManager(const SceneAssetManager&) = delete;
		SceneAssetManager& operator=(const SceneAssetManager&) = delete;
		SceneAssetManager(SceneAssetManager&&) = delete;
		SceneAssetManager& operator=(SceneAssetManager&&) = delete;

		SceneAssetLoadResult LoadSceneAsset(const SceneAssetId& sceneAssetId);
		SceneAssetLoadResult LoadSceneAssets(std::span<const SceneAssetId> sceneAssetIds);
		void UnloadAll() noexcept;

	  private:
		static bool AppendSceneAssetToPayload(
		    const SceneAssetId& sceneAssetId,
		    RuntimeScenePayload& payload,
		    std::uint32_t& materialBaseIndex,
		    std::string& errorMessage);
		static std::filesystem::path GetCookedAssetRootPath();
		static std::filesystem::path BuildSceneManifestPath(const SceneAssetId& sceneAssetId);
		static std::filesystem::path BuildMeshAssetPath(CookedAssetId meshAssetId);
		static std::filesystem::path BuildMaterialAssetPath(CookedAssetId materialAssetId);
	};
}