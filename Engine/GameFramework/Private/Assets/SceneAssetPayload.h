#pragma once

#include "Assets/Cooked/CookedAssetCommon.h"
#include "Assets/SceneMaterialVariantPayload.h"
#include "Assets/SceneMeshAssetPayload.h"
#include "Scene/Animations/AnimationClipResource.h"
#include "Scene/Camera/CameraDesc.h"
#include "Scene/Lighting/SceneLightDesc.h"
#include "Scene/Materials/MaterialDesc.h"
#include "Scene/Skeletons/SkeletonResource.h"

#include <string>
#include <vector>

struct SceneAssetPayload final
{
	using StaticMeshAsset = SceneAssetStaticMeshAsset;
	using SkeletalMeshAsset = SceneAssetSkeletalMeshAsset;
	using StaticMeshInstance = SceneAssetStaticMeshInstance;
	using SkeletalMeshInstance = SceneAssetSkeletalMeshInstance;
	using MeshInstanceGroup = SceneAssetMeshInstanceGroup;
	using MaterialVariant = SceneAssetMaterialVariant;
	using MaterialVariantMapping = SceneAssetMaterialVariantMapping;

	struct Camera final
	{
		std::string name;
		CameraDesc desc;

		bool IsPerspective() const noexcept { return desc.projectionKind == CameraProjectionKind::Perspective; }
	};

	std::vector<StaticMeshAsset> staticMeshAssets;
	std::vector<SkeletalMeshAsset> skeletalMeshAssets;
	std::vector<StaticMeshInstance> staticMeshInstances;
	std::vector<SkeletalMeshInstance> skeletalMeshInstances;
	std::vector<MeshInstanceGroup> meshInstanceGroups;
	std::vector<Camera> cameras;
	std::vector<SceneLightDesc> lights;
	std::vector<SkeletonResource> skeletons;
	std::vector<AnimationClipResource> animations;
	std::vector<MaterialDesc> materials;
	std::vector<MaterialVariant> materialVariants;
	std::vector<MaterialVariantMapping> materialVariantMappings;
	bool HasMeshes() const noexcept
	{
		return (!staticMeshAssets.empty() && !staticMeshInstances.empty()) ||
		       (!skeletalMeshAssets.empty() && !skeletalMeshInstances.empty());
	}
};
