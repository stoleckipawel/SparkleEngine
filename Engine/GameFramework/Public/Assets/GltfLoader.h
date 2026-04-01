#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Assets/SceneImportResult.h"

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
	static SceneImportResult Load(const std::filesystem::path& filePath);

	GltfLoader() = delete;
	~GltfLoader() = delete;

  private:
	struct CgltfGuard
	{
		cgltf_data* ptr = nullptr;
		~CgltfGuard();
	};

	static bool ValidateInputPath(const std::filesystem::path& filePath, SceneImportResult& result);
	static bool ParseGltfFile(
	    cgltf_options& options,
	    const std::string& pathStr,
	    cgltf_data*& outData,
	    SceneImportResult& result);
	static bool LoadGltfBuffers(
	    cgltf_options& options,
	    cgltf_data* data,
	    const std::string& pathStr,
	    SceneImportResult& result);
	static void ValidateGltf(cgltf_data* data, const std::string& pathStr, SceneImportResult& result);
	static std::size_t CountTotalPrimitives(const cgltf_data* data);
	static void ExtractMeshesFromNodes(const cgltf_data* data, SceneImportResult& result);
	template <typename T> static T ReadAccessorElement(const cgltf_accessor* accessor, std::size_t index);
	static const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive, int type);
	static void ReadIndices(const cgltf_accessor* accessor, std::vector<std::uint32_t>& outIndices);
	static DirectX::XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node);
	static std::filesystem::path ResolveImagePath(const cgltf_image* image, const std::filesystem::path& gltfDirectory);

	static void ExtractMaterials(
	    const cgltf_data* data,
	    const std::filesystem::path& gltfDirectory,
	    std::vector<MaterialDesc>& outMaterials);
		
	static std::uint32_t ResolveMaterialOffset(const cgltf_primitive& primitive, const cgltf_data* data);
	static MeshData ExtractPrimitive(const cgltf_primitive& primitive);
	static std::string BuildPrimitiveLabel(const cgltf_node& node, std::size_t primitiveIndex);
};
