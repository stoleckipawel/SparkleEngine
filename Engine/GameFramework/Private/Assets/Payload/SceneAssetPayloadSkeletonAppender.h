#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

#include <string>

namespace Assets
{
	class CookedAssetFileSet;
	class SceneAssetPayloadSkeletonAppender final
	{
	  public:
		static bool AppendSkeletons(
		    const LoadedSceneManifest& sceneManifest,
		    const CookedAssetFileSet& files,
		    SceneAssetPayload& sceneAssetPayload,
		    std::string& errorMessage);
	};
}
