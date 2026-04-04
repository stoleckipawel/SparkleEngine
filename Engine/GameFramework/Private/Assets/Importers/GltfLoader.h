#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <DirectXMath.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct cgltf_accessor;
struct cgltf_data;
struct cgltf_material;
struct cgltf_node;
struct cgltf_options;
struct cgltf_primitive;
struct cgltf_texture_view;

class GltfLoader final
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
	static bool ParseGltfFile(cgltf_options& options, const std::string& pathStr, cgltf_data*& outData, SceneImportResult& result);
	static bool LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& pathStr, SceneImportResult& result);
	static void ValidateGltf(cgltf_data* data, const std::string& pathStr, SceneImportResult& result);
	static void CollectSceneWarnings(const cgltf_data* data, SceneImportResult& result);
	static std::size_t CountTotalPrimitives(const cgltf_data* data);
	static void ExtractMeshesFromNodes(const cgltf_data* data, SceneImportResult& result);
	template <typename T> static T ReadAccessorElement(const cgltf_accessor* accessor, std::size_t index);
	static const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive, int type);
	static void ReadIndices(const cgltf_accessor* accessor, std::vector<std::uint32_t>& outIndices);
	static DirectX::XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node);
	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const cgltf_texture_view& textureView,
	    const std::filesystem::path& gltfDirectory,
	    std::string_view materialName,
	    std::string_view slotName,
	    SceneImportResult& result);
	static void AppendUnsupportedMaterialWarnings(const cgltf_material& material, std::string_view materialName, SceneImportResult& result);

	static void ExtractMaterials(const cgltf_data* data, const std::filesystem::path& gltfDirectory, SceneImportResult& result);

	static std::uint32_t ResolveMaterialOffset(const cgltf_primitive& primitive, const cgltf_data* data);
	static MeshData ExtractPrimitive(const cgltf_primitive& primitive);
	static std::string BuildPrimitiveLabel(const cgltf_node& node, std::size_t primitiveIndex);
};
