#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Assets/SceneMeshAssetPayload.h"
#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"
#include "GameFramework/Public/Scene/Skeletons/SceneSkeleton.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct SPARKLE_ENGINE_API SceneAssetPayloadDiagnostics
{
	std::size_t loadedSceneAssetCount = 0;
	std::size_t meshAssetReferenceCount = 0;
	std::size_t meshInstanceCount = 0;
	std::size_t meshInstanceGroupCount = 0;
	std::size_t cameraCount = 0;
	std::size_t lightCount = 0;
	std::size_t skeletonRefCount = 0;
	std::size_t animationRefCount = 0;
	std::uint32_t sceneFeatureFlags = 0;
};

struct SPARKLE_ENGINE_API SceneAssetPayload
{
	using StaticMeshAsset = SceneAssetStaticMeshAsset;
	using SkeletalMeshAsset = SceneAssetSkeletalMeshAsset;
	using StaticMeshInstance = SceneAssetStaticMeshInstance;
	using SkeletalMeshInstance = SceneAssetSkeletalMeshInstance;
	using MeshInstanceGroup = SceneAssetMeshInstanceGroup;

	struct Camera
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
	std::vector<SceneSkeletonDesc> skeletons;
	std::vector<SceneAnimationClipDesc> animations;
	std::vector<MaterialDesc> materials;
	SceneAssetPayloadDiagnostics diagnostics;

	bool HasMeshes() const noexcept;
	std::size_t GetMeshCount() const noexcept;
	std::size_t GetMaterialCount() const noexcept;
};
