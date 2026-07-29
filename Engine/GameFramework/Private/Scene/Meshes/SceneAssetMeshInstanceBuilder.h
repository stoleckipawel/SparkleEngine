#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Scene/Meshes/MeshInstanceGroup.h"
#include "World/SceneMeshInstanceData.h"

#include <vector>

class MaterialResourceStore;

namespace SceneAssetMeshInstanceBuilder
{
	std::vector<ECS::SceneMeshInstanceData> BuildInstances(
	    SceneAssetPayload& payload,
	    MaterialResourceStore& materials,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceGroupIndex groupBaseIndex);

	std::vector<SceneMeshInstanceGroupData> BuildGroups(
	    const SceneAssetPayload& payload,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceIndex meshBaseIndex);
}
