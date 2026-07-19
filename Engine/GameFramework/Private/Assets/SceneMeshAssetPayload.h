#pragma once

#include "Assets/Cooked/CookedAssetCommon.h"
#include "Assets/Cooked/CookedMeshAsset.h"
#include "Assets/Cooked/CookedSceneManifest.h"
#include "Scene/Materials/MaterialHandle.h"
#include "Scene/Meshes/MeshData.h"
#include "Scene/Meshes/MeshInstanceGroup.h"
#include "Scene/Meshes/SkeletalMeshData.h"
#include "Scene/Meshes/StaticMeshData.h"
#include "Scene/Transform.h"

#include <cstdint>
#include <vector>

struct SceneAssetStaticMeshAsset final
{
	StaticMeshData mesh;
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
};

struct SceneAssetSkeletalMeshAsset final
{
	SkeletalMeshData mesh;
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
};

struct SceneAssetStaticMeshInstance final
{
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	Transform transform;
	MaterialHandle material;
	std::uint32_t sourceNodeIndex = Assets::kInvalidCookedSceneSourceNodeIndex;
	SceneMeshInstanceGroupIndex groupIndex = kInvalidSceneMeshInstanceGroupIndex;
};

struct SceneAssetSkeletalMeshInstance final
{
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	Transform transform;
	MaterialHandle material;
	Assets::CookedAssetId skeletonAssetId = Assets::InvalidCookedAssetId;
	std::uint32_t sourceNodeIndex = Assets::kInvalidCookedSceneSourceNodeIndex;
	std::vector<float> morphWeights;
};

struct SceneAssetMeshInstanceGroup final
{
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	Assets::CookedMeshAssetKind meshAssetKind = Assets::CookedMeshAssetKind::Static;
	MaterialHandle material;
	SceneMeshInstanceIndex firstInstance = kInvalidSceneMeshInstanceIndex;
	std::uint32_t instanceCount = 0;
	SceneMeshInstanceGroupKind groupKind = SceneMeshInstanceGroupKind::None;
	std::uint32_t flags = 0;
};
