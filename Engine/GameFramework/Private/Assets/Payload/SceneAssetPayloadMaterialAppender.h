#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

namespace Assets
{
	class CookedAssetFileSet;
	class SceneAssetPayloadMaterialAppender final
	{
	public:
		static void AppendMaterials(
		    const LoadedSceneManifest& sceneManifest,
		    CookedAssetFileSet& files,
		    SceneAssetPayload& sceneAssetPayload);
	};
}
