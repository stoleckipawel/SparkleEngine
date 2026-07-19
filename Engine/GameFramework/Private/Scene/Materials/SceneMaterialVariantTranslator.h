#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Scene/Meshes/MeshInstanceGroup.h"
#include "World/Resources/MaterialVariantResourceStore.h"

#include <vector>

namespace ECS
{
	class GameWorldState;
}

namespace SceneMaterialVariantTranslator
{
	std::vector<MaterialVariantDesc> BuildDescriptions(const SceneAssetPayload& sceneAssetPayload);

	std::vector<MaterialVariantBinding> BuildBindings(
	    const SceneAssetPayload& sceneAssetPayload,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceIndex sceneMeshBaseIndex,
	    const ECS::GameWorldState& world);
}
