#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Level/LevelDesc.h"

#include <string>

namespace Assets
{
	struct LoadedSceneManifest;
	class CookedAssetFileSet;

	class SceneAssetPayloadDecoder final
	{
	  public:
		static bool Decode(
		    const SceneAssetId& sceneAssetId,
		    LoadedSceneManifest& manifest,
		    const CookedAssetFileSet& files,
		    SceneAssetPayload& sceneAssetPayload,
		    std::string& errorMessage);
	};
}
