#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <assimp/scene.h>

#include <cstddef>
#include <string>

class FbxGeometryImporter final
{
  public:
	static std::size_t CountImportedMeshInstances(const aiNode& node) noexcept;
	static void ImportGeometry(const aiScene& scene, SceneImportResult& result);

  private:
	static void ExtractNodeMeshes(const aiScene& scene, const aiNode& node, const aiMatrix4x4& parentTransform, SceneImportResult& result);
	static void AppendMeshInstance(const aiNode& node, const aiMesh& mesh, const aiMatrix4x4& worldTransform, SceneImportResult& result);
	static MeshData ExtractMeshGeometry(const aiMesh& mesh, const aiNode& node, SceneImportResult& result);
	static void PopulateVertices(const aiMesh& mesh, MeshData& meshData);
	static void AppendTriangleIndices(const aiMesh& mesh, MeshData& meshData, SceneImportResult& result);
	static MaterialHandle ResolveMaterialHandle(const aiMesh& mesh, SceneImportResult& result) noexcept;
	static std::string GetNodeName(const aiNode& node);
	static std::string GetMeshName(const aiMesh& mesh);
	static Transform ConvertTransform(const aiMatrix4x4& matrix) noexcept;
};