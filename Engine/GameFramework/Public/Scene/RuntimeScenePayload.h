#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshData.h"
#include "GameFramework/Public/Scene/Transform.h"

#include <cstddef>
#include <vector>

struct SPARKLE_ENGINE_API RuntimeScenePayload
{
	std::vector<MeshData> meshes;
	std::vector<MaterialDesc> materials;
	std::vector<Transform> transforms;
	std::vector<MaterialHandle> materialHandles;

	bool HasMeshes() const noexcept;
	std::size_t GetMeshCount() const noexcept;
	std::size_t GetMaterialCount() const noexcept;

	void Reserve(std::size_t meshCount);
};