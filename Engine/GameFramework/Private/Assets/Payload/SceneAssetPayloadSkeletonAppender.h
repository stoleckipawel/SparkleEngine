#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

#include <string>

namespace Assets
{
	class SceneAssetPayloadSkeletonAppender final
	{
	  public:
		static bool AppendSkeletons(
		    const LoadedSceneManifest& sceneManifest,
		    SceneAssetPayload& sceneAssetPayload,
		    std::string& errorMessage);
	};
}
