#pragma once

#include "Assets/Payload/SceneAssetPayloadMeshBindings.h"

#include <span>
#include <string>

namespace Assets
{
	class SceneAssetPayloadMaterialVariantAppender final
	{
	  public:
		static bool AppendMaterialVariants(
		    const LoadedSceneManifest& sceneManifest,
		    std::span<const SceneAssetPayloadMeshBinding> meshAssetBindings,
		    SceneAssetPayload& sceneAssetPayload,
		    std::uint32_t materialBaseIndex,
		    std::string& errorMessage);
	};
}
