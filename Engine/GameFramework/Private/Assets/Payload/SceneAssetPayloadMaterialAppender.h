#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

#include <string>

namespace Assets
{
	class SceneAssetPayloadMaterialAppender final
	{
	  public:
		static bool AppendMaterials(
		    const LoadedSceneManifest& sceneManifest,
		    SceneAssetPayload& sceneAssetPayload,
		    std::string& errorMessage);
	};
}
