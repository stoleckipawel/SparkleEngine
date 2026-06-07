#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Scene/Materials/SceneMaterialVariants.h"
#include "Scene/Meshes/MeshInstanceGroup.h"

#include <vector>

namespace SceneMaterialVariantPayloadApplier
{
	std::vector<SceneMaterialVariantDesc> BuildVariantDescs(const SceneAssetPayload& sceneAssetPayload);

	std::vector<SceneMaterialVariantBinding> BuildVariantBindings(
	    const SceneAssetPayload& sceneAssetPayload,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceIndex sceneMeshBaseIndex);
}
