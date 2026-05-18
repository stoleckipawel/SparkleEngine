#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshData.h"
#include "GameFramework/Public/Scene/Transform.h"

#include <cstddef>
#include <vector>

struct SPARKLE_ENGINE_API SceneAssetPayloadDiagnostics
{
	std::size_t loadedSceneAssetCount = 0;
	std::size_t meshAssetReferenceCount = 0;
	std::size_t meshInstanceCount = 0;
	std::size_t meshInstanceGroupCount = 0;
};

struct SPARKLE_ENGINE_API SceneAssetPayload
{
	struct MeshInstance
	{
		MeshData mesh;
		Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
		Transform transform;
		MaterialHandle material;
	};

	std::vector<MeshInstance> meshInstances;
	std::vector<MaterialDesc> materials;
	SceneAssetPayloadDiagnostics diagnostics;

	bool HasMeshes() const noexcept;
	std::size_t GetMeshCount() const noexcept;
	std::size_t GetMaterialCount() const noexcept;
};