#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshData.h"
#include "GameFramework/Public/Scene/Transform.h"

#include <cstddef>
#include <vector>

struct SPARKLE_ENGINE_API ImportedSceneData
{
	std::vector<MeshData> meshes;
	std::vector<MaterialDesc> materials;
	std::vector<Transform> transforms;
	std::vector<MaterialHandle> materialHandles;

	bool HasMeshes() const noexcept { return !meshes.empty(); }
	std::size_t GetMeshCount() const noexcept { return meshes.size(); }
	std::size_t GetMaterialCount() const noexcept { return materials.size(); }

	void Reserve(std::size_t meshCount)
	{
		meshes.reserve(meshCount);
		transforms.reserve(meshCount);
		materialHandles.reserve(meshCount);
	}
};