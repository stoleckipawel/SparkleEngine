#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/MeshSnapshot.h"

#include <memory>
#include <vector>

class SceneMaterials;

namespace SceneAssetMeshComponentFactory
{
	bool BuildMeshComponents(
	    SceneAssetPayload& sceneAssetPayload,
	    SceneMaterials& sceneMaterials,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceGroupIndex sceneGroupBaseIndex,
	    std::vector<std::unique_ptr<MeshComponent>>& outMeshComponents);

	std::vector<MeshInstanceGroupSnapshot> BuildMeshInstanceGroups(
	    const SceneAssetPayload& sceneAssetPayload,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceIndex sceneMeshBaseIndex);
}
