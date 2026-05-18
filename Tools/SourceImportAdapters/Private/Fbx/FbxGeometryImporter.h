#pragma once

#include "SourceImportResult.h"

#include <assimp/scene.h>

#include <cstddef>
#include <cstdint>
#include <string>

class FbxGeometryImporter final
{
  public:
	static std::size_t CountImportedMeshInstances(const aiNode& node) noexcept;
	static void ImportGeometry(const aiScene& scene, SourceImportResult& result);

  private:
	static void ExtractNodeMeshes(const aiScene& scene, const aiNode& node, const aiMatrix4x4& parentTransform, SourceImportResult& result);
	static void AppendMeshInstance(
	    const aiNode& node,
	    const aiMesh& mesh,
	    std::uint32_t sourceMeshIndex,
	    const aiMatrix4x4& worldTransform,
	    SourceImportResult& result);
	static ImportedMeshGeometry ExtractMeshGeometry(const aiMesh& mesh, const aiNode& node, SourceImportResult& result);
	static void PopulateVertices(const aiMesh& mesh, ImportedMeshGeometry& meshGeometry);
	static void AppendTriangleIndices(const aiMesh& mesh, ImportedMeshGeometry& meshGeometry, SourceImportResult& result);
	static ImportedMaterialIndex ResolveMaterialIndex(const aiMesh& mesh, SourceImportResult& result) noexcept;
	static ImportedMeshPrimitiveIndex FindImportedPrimitiveIndex(const ImportedScene& scene, std::uint32_t sourceMeshIndex) noexcept;
	static std::string BuildMeshDisplayName(const aiNode& node, const aiMesh& mesh);
	static std::string GetNodeName(const aiNode& node);
	static std::string GetMeshName(const aiMesh& mesh);
	static DirectX::XMFLOAT4X4 ConvertTransform(const aiMatrix4x4& matrix) noexcept;
};


