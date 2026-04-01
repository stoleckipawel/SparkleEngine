#pragma once

#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/MeshData.h"
#include "GameFramework/Public/Scene/Transform.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

struct SPARKLE_ENGINE_API SceneImportWarning
{
	std::string message;
};

struct SPARKLE_ENGINE_API SceneImportResult
{
	std::vector<MeshData> meshes;
	std::vector<MaterialDesc> materials;
	std::vector<Transform> transforms;
	std::vector<std::uint32_t> materialOffsets;
	std::vector<SceneImportWarning> warnings;

	bool bSuccess = false;
	std::string errorMessage;

	bool IsValid() const noexcept { return bSuccess && !meshes.empty(); }
	bool HasWarnings() const noexcept { return !warnings.empty(); }
	std::size_t GetMeshCount() const noexcept { return meshes.size(); }
	std::size_t GetMaterialCount() const noexcept { return materials.size(); }

	void AddWarning(std::string warningMessage)
	{
		warnings.push_back({std::move(warningMessage)});
	}
	
	void Reserve(std::size_t primitiveCount)
	{
		meshes.reserve(primitiveCount);
		transforms.reserve(primitiveCount);
		materialOffsets.reserve(primitiveCount);
	}
};