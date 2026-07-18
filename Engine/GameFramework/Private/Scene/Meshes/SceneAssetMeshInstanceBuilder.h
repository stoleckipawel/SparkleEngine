#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Scene/Meshes/MeshSnapshot.h"
#include "World/SceneMeshInstanceData.h"

#include <vector>

class SceneMaterials;

namespace SceneAssetMeshInstanceBuilder
{
	bool BuildInstances(
	    SceneAssetPayload& payload,
	    SceneMaterials& materials,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceGroupIndex groupBaseIndex,
	    std::vector<ECS::SceneMeshInstanceData>& outInstances);

	std::vector<MeshInstanceGroupSnapshot> BuildGroups(
	    const SceneAssetPayload& payload,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceIndex meshBaseIndex);
}
