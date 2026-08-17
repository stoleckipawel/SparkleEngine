#pragma once

#include "Level/LevelDesc.h"

#include <cstddef>
#include <filesystem>
namespace Assets
{
	struct LoadedSceneManifest;
	class CookedAssetFileSet;
	class SceneLoadBudget;

	class SceneAssetFileReader final
	{
	public:
		static std::size_t Read(
		    const SceneAssetId& sceneAssetId,
		    const std::filesystem::path& manifestRelativePath,
		    LoadedSceneManifest& manifest,
		    CookedAssetFileSet& files,
		    SceneLoadBudget& budget);
	};
}
