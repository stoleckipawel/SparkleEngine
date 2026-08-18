#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

namespace Assets
{
	class CookedAssetFileSet;
	class SceneAssetPayloadAnimationAppender final
	{
	public:
		static void AppendAnimations(
		    const LoadedSceneManifest& sceneManifest,
		    CookedAssetFileSet& files,
		    SceneAssetPayload& sceneAssetPayload);
	};
}
