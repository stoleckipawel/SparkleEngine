#pragma once

#include "Level/Loading/SceneLoadExecutor.h"
#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetFileSet.h"

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

namespace Assets
{
	struct SceneAssetLoadWork final
	{
		SceneAssetId Id;
		std::filesystem::path ManifestPath;
		LoadedSceneManifest Manifest;
		CookedAssetFileSet Files;
		SceneAssetPayload Payload;
		std::vector<EntityBlueprint> Entities;
	};

	struct SceneLoadWorkState final
	{
		std::unique_ptr<SceneLoadPackage> Package;
		std::vector<SceneAssetLoadWork> Assets;
		std::atomic<std::size_t> RetainedBytes = 0;
		std::atomic<std::uint32_t> CompletedDecodes = 0;
		std::atomic<LevelLoadOperationStage> Stage = LevelLoadOperationStage::Reading;
	};
}
