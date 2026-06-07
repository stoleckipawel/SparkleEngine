#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Assets/Cooked/CookedMeshAsset.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshData.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"
#include "GameFramework/Public/Scene/Meshes/SkeletalMeshData.h"
#include "GameFramework/Public/Scene/Meshes/StaticMeshData.h"
#include "GameFramework/Public/Scene/Transform.h"

#include <cstdint>
#include <vector>

struct SPARKLE_ENGINE_API SceneAssetStaticMeshAsset
{
	StaticMeshData mesh;
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
};

struct SPARKLE_ENGINE_API SceneAssetSkeletalMeshAsset
{
	SkeletalMeshData mesh;
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
};

struct SPARKLE_ENGINE_API SceneAssetStaticMeshInstance
{
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	Transform transform;
	MaterialHandle material;
	SceneMeshInstanceGroupIndex groupIndex = kInvalidSceneMeshInstanceGroupIndex;
};

struct SPARKLE_ENGINE_API SceneAssetSkeletalMeshInstance
{
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	Transform transform;
	MaterialHandle material;
	Assets::CookedAssetId skeletonAssetId = Assets::InvalidCookedAssetId;
	std::vector<float> morphWeights;
};

struct SPARKLE_ENGINE_API SceneAssetMeshInstanceGroup
{
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	Assets::CookedMeshAssetKind meshAssetKind = Assets::CookedMeshAssetKind::Static;
	MaterialHandle material;
	SceneMeshInstanceIndex firstInstance = kInvalidSceneMeshInstanceIndex;
	std::uint32_t instanceCount = 0;
	SceneMeshInstanceGroupKind groupKind = SceneMeshInstanceGroupKind::None;
	std::uint32_t flags = 0;
};
