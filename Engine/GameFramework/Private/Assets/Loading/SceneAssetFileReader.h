#pragma once

#include "Level/LevelDesc.h"

#include <filesystem>
namespace Assets
{
	struct LoadedSceneManifest;
	class CookedAssetFileSet;

	class SceneAssetFileReader final
	{
	public:
		static void Read(
		    const SceneAssetId& sceneAssetId,
		    const std::filesystem::path& manifestRelativePath,
		    LoadedSceneManifest& manifest,
		    CookedAssetFileSet& files);
	};
}
