#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshData.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"
#include "GameFramework/Public/Scene/Skeletons/SceneSkeleton.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"
#include "GameFramework/Public/Scene/Transform.h"

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
	struct MeshAsset
	{
		MeshData mesh;
		Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	};

	struct MeshInstance
	{
		SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
		Transform transform;
		MaterialHandle material;
		SceneMeshInstanceGroupIndex groupIndex = kInvalidSceneMeshInstanceGroupIndex;
		Assets::CookedAssetId skeletonAssetId = Assets::InvalidCookedAssetId;
	};

	struct MeshInstanceGroup
	{
		SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
		MaterialHandle material;
		SceneMeshInstanceIndex firstInstance = kInvalidSceneMeshInstanceIndex;
		std::uint32_t instanceCount = 0;
		SceneMeshInstanceGroupKind groupKind = SceneMeshInstanceGroupKind::None;
		std::uint32_t flags = 0;
	};

	struct Camera
	{
		std::string name;
		CameraDesc desc;

		bool IsPerspective() const noexcept { return desc.projectionKind == CameraProjectionKind::Perspective; }
	};

	std::vector<MeshAsset> meshAssets;
	std::vector<MeshInstance> meshInstances;
	std::vector<MeshInstanceGroup> meshInstanceGroups;
	std::vector<Camera> cameras;
	std::vector<SceneLightDesc> lights;
	std::vector<SceneSkeletonDesc> skeletons;
	std::vector<MaterialDesc> materials;
	SceneAssetPayloadDiagnostics diagnostics;

	bool HasMeshes() const noexcept;
	std::size_t GetMeshCount() const noexcept;
	std::size_t GetMaterialCount() const noexcept;
};
