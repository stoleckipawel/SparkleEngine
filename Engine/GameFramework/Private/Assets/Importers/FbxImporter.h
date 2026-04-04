#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <assimp/material.h>
#include <assimp/scene.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace Assimp
{
	class Importer;
}

class FbxImporter final
{
  public:
	static SceneImportResult Load(const std::filesystem::path& filePath);

	FbxImporter() = delete;
	~FbxImporter() = delete;

  private:
	static constexpr unsigned int GetPostProcessFlags() noexcept;

	static void ConfigureImporter(Assimp::Importer& importer);

	static bool ValidateInputPath(const std::filesystem::path& filePath, SceneImportResult& result);

	static bool TryReadScene(
	    Assimp::Importer& importer,
	    const std::filesystem::path& filePath,
	    const aiScene*& scene,
	    SceneImportResult& result);
	static void PrepareResultStorage(const aiScene& scene, SceneImportResult& result);
	static std::size_t CountNodeMeshInstances(const aiNode& node) noexcept;

	static void CollectSceneWarnings(const aiScene& scene, SceneImportResult& result);
	static void CollectMaterialWarnings(const aiMaterial& material, std::string_view materialName, SceneImportResult& result);

	static void ExtractMaterials(const aiScene& scene, const std::filesystem::path& sourceDirectory, SceneImportResult& result);

	static MaterialDesc ExtractMaterial(
	    const aiMaterial& material,
	    unsigned int materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    SceneImportResult& result);

	static void ApplyMaterialProperties(const aiMaterial& material, MaterialDesc& materialDesc);

	static void ApplyTextureMappings(
	    const aiMaterial& material,
	    const std::filesystem::path& sourceDirectory,
	    MaterialDesc& materialDesc,
	    SceneImportResult& result);

	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const aiMaterial& material,
	    const std::filesystem::path& sourceDirectory,
	    aiTextureType textureType,
	    std::string_view slotName,
	    const std::string& materialName,
	    SceneImportResult& result);

	static std::string GetMaterialName(const aiMaterial& material, unsigned int materialIndex);

	static void ExtractNodeMeshes(const aiScene& scene, const aiNode& node, const aiMatrix4x4& parentTransform, SceneImportResult& result);

	static void AppendMeshInstance(const aiNode& node, const aiMesh& mesh, const aiMatrix4x4& worldTransform, SceneImportResult& result);

	static MeshData ExtractMeshGeometry(const aiMesh& mesh, const aiNode& node, SceneImportResult& result);

	static void PopulateVertices(const aiMesh& mesh, MeshData& meshData);

	static void AppendTriangleIndices(const aiMesh& mesh, MeshData& meshData, SceneImportResult& result);

	static std::uint32_t ResolveMaterialOffset(const aiMesh& mesh, SceneImportResult& result) noexcept;

	static std::string GetNodeName(const aiNode& node);

	static std::string GetMeshName(const aiMesh& mesh);

	static Transform ConvertTransform(const aiMatrix4x4& matrix) noexcept;
};