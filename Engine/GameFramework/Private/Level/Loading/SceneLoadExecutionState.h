#pragma once

#include "Level/Loading/SceneLoadExecutionService.h"
#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetFileSet.h"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
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
		std::string Error;
	};

	struct SceneLoadSharedState final
	{
		std::unique_ptr<SceneLoadPackage> Package;
		std::vector<SceneAssetLoadWork> Assets;
		std::atomic<std::size_t> RetainedBytes = 0;
		std::atomic<std::uint32_t> CompletedDecodes = 0;
		std::atomic<LevelLoadOperationStage> Stage = LevelLoadOperationStage::Reading;
	};
}
