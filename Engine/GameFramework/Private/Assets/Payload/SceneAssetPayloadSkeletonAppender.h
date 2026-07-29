#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

namespace Assets
{
	class CookedAssetFileSet;
	class SceneAssetPayloadSkeletonAppender final
	{
	  public:
		static void AppendSkeletons(
		    const LoadedSceneManifest& sceneManifest,
		    const CookedAssetFileSet& files,
		    SceneAssetPayload& sceneAssetPayload);
	};
}
