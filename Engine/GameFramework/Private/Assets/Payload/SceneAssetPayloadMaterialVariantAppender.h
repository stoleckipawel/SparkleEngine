#pragma once

#include "Assets/Payload/SceneAssetPayloadMeshBindings.h"

#include <span>
namespace Assets
{
	class SceneAssetPayloadMaterialVariantAppender final
	{
	  public:
		static void AppendMaterialVariants(
		    const LoadedSceneManifest& sceneManifest,
		    std::span<const SceneAssetPayloadMeshBinding> meshAssetBindings,
		    SceneAssetPayload& sceneAssetPayload);
	};
}
