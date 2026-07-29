#pragma once

#include "SourceImportOutput.h"

#include <assimp/scene.h>

#include <cstddef>
#include <cstdint>
#include <string>

class FbxGeometryImporter final
{
  public:
	static std::size_t CountImportedMeshInstances(const aiNode& node) noexcept;
	static void ImportGeometry(const aiScene& scene, SourceImportOutput& output);

  private:
	static void ExtractNodeMeshes(
	    const aiScene& scene,
	    const aiNode& node,
	    const aiMatrix4x4& parentTransform,
	    std::uint32_t& nextNodeIndex,
	    SourceImportOutput& output);
	static void AppendMeshInstance(
	    const aiScene& scene,
	    const aiNode& node,
	    const aiMesh& mesh,
	    std::uint32_t sourceMeshIndex,
	    std::uint32_t sourceNodeIndex,
	    const aiMatrix4x4& worldTransform,
	    SourceImportOutput& output);
	static ImportedMeshGeometry ExtractMeshGeometry(
	    const aiMesh& mesh,
	    const aiNode& node,
	    const ImportedSkeleton* skeleton,
	    SourceImportOutput& output);
	static void PopulateVertices(const aiMesh& mesh, ImportedMeshGeometry& meshGeometry);
	static void AppendTriangleIndices(const aiMesh& mesh, ImportedMeshGeometry& meshGeometry);
	static ImportedMaterialIndex ResolveMaterialIndex(const aiMesh& mesh, const SourceImportOutput& output);
	static ImportedMeshPrimitiveIndex FindImportedPrimitiveIndex(const ImportedScene& scene, std::uint32_t sourceMeshIndex) noexcept;
	static std::string BuildMeshDisplayName(const aiNode& node, const aiMesh& mesh);
	static std::string GetNodeName(const aiNode& node);
	static std::string GetMeshName(const aiMesh& mesh);
};
