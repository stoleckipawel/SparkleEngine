#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/Scene/MeshData.h"

#include <DirectXMath.h>
#include <filesystem>
#include <string>
#include <vector>

struct cgltf_accessor;
struct cgltf_data;
struct cgltf_image;
struct cgltf_node;
struct cgltf_options;
struct cgltf_primitive;

class SPARKLE_ENGINE_API GltfLoader final
{
  public:
	struct LoadResult
	{
		std::vector<MeshData> meshes;
		std::vector<MaterialDesc> materials;
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

  private:
	struct CgltfGuard
	{
		cgltf_data* ptr = nullptr;
		~CgltfGuard();
	};

	static bool ValidateInputPath(const std::filesystem::path& filePath, LoadResult& result);
	static bool ParseGltfFile(cgltf_options& options, const std::string& pathStr, cgltf_data*& outData, LoadResult& result);
	static bool LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& pathStr, LoadResult& result);
	static void ValidateGltf(cgltf_data* data, const std::string& pathStr);
	static std::size_t CountTotalPrimitives(const cgltf_data* data);
	static void ExtractMeshesFromNodes(const cgltf_data* data, LoadResult& result);
	template <typename T> static T ReadAccessorElement(const cgltf_accessor* accessor, std::size_t index);
	static const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive, int type);
	static void ReadIndices(const cgltf_accessor* accessor, std::vector<std::uint32_t>& outIndices);
	static DirectX::XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node);
	static std::filesystem::path ResolveImagePath(const cgltf_image* image, const std::filesystem::path& gltfDirectory);

	static void ExtractMaterials(
	    const cgltf_data* data,
	    const std::filesystem::path& gltfDirectory,
	    std::vector<MaterialDesc>& outMaterials);
		
	static std::uint32_t ResolveMaterialIndex(const cgltf_primitive& primitive, const cgltf_data* data);
	static MeshData ExtractPrimitive(const cgltf_primitive& primitive);
};
