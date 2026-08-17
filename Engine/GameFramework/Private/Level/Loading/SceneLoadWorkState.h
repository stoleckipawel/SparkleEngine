#pragma once

#include "Level/Loading/SceneLoadExecutor.h"
#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetFileSet.h"
#include "Level/Loading/SceneLoadBudget.h"

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
		std::size_t RetainedManifestBytes = 0;
		std::size_t RetainedDecodedBytes = 0;
	};

	struct SceneLoadWorkState final
	{
		std::unique_ptr<SceneLoadPackage> Package;
		std::vector<SceneAssetLoadWork> Assets;
		SceneLoadBudget Budget;
		std::atomic<std::uint32_t> CompletedDecodes = 0;
		std::atomic<LevelLoadOperationStage> Stage = LevelLoadOperationStage::Reading;
	};
}
