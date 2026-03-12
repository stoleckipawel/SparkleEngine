#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/Scene/MeshData.h"

#include <DirectXMath.h>
#include <filesystem>
#include <string>
#include <vector>

class SPARKLE_ENGINE_API GltfLoader final
{
  public:
	struct LoadResult
	{
		std::vector<MeshData> meshes;
		std::vector<MaterialDesc> materials;
		std::vector<std::string> texturePaths;
		std::vector<DirectX::XMFLOAT4X4> transforms;
		std::vector<std::uint32_t> materialIndices;

		bool bSuccess = false;
		std::string errorMessage;

		bool IsValid() const noexcept { return bSuccess && !meshes.empty(); }
		std::size_t GetMeshCount() const noexcept { return meshes.size(); }
		std::size_t GetMaterialCount() const noexcept { return materials.size(); }
	};

	static LoadResult Load(const std::filesystem::path& filePath);

	GltfLoader() = delete;
	~GltfLoader() = delete;
};
