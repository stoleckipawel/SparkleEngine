#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

#include <string>

namespace Assets
{
	class SceneAssetPayloadMaterialVariantAppender final
	{
	  public:
		static bool AppendMaterialVariants(
		    const LoadedSceneManifest& sceneManifest,
		    SceneAssetPayload& sceneAssetPayload,
		    std::uint32_t materialBaseIndex,
		    std::string& errorMessage);
	};
}
