#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

#include <string>

namespace Assets
{
	class SceneAssetPayloadAnimationAppender final
	{
	  public:
		static bool AppendAnimations(
		    const LoadedSceneManifest& sceneManifest,
		    SceneAssetPayload& sceneAssetPayload,
		    std::string& errorMessage);
	};
}
