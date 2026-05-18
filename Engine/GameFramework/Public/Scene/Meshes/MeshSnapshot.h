#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"

#include <DirectXMath.h>

#include <vector>

class Mesh;

struct SPARKLE_ENGINE_API MeshInstanceSnapshot
{
	const Mesh* mesh = nullptr;
	DirectX::XMFLOAT4X4 worldMatrix = {};
	DirectX::XMFLOAT3X4 worldInvTranspose = {};
	MaterialHandle materialHandle = MaterialHandle::Invalid();
	Assets::CookedAssetId meshAssetId = Assets::InvalidCookedAssetId;
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	SceneMeshInstanceGroupIndex instanceGroupIndex = kInvalidSceneMeshInstanceGroupIndex;
};

struct SPARKLE_ENGINE_API MeshInstanceGroupSnapshot
{
	Assets::CookedAssetId meshAssetId = Assets::InvalidCookedAssetId;
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	MaterialHandle materialHandle = MaterialHandle::Invalid();
	SceneMeshInstanceIndex firstInstance = kInvalidSceneMeshInstanceIndex;
	std::uint32_t instanceCount = 0;
	SceneMeshInstanceGroupKind groupKind = SceneMeshInstanceGroupKind::None;
	std::uint32_t flags = 0;
};

struct SPARKLE_ENGINE_API MeshSnapshot
{
	std::vector<MeshInstanceSnapshot> meshInstances;
	std::vector<MeshInstanceGroupSnapshot> meshInstanceGroups;

	bool HasMeshes() const noexcept { return !meshInstances.empty(); }
	void Reset() noexcept
	{
		meshInstances.clear();
		meshInstanceGroups.clear();
	}
};