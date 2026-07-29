#pragma once

#include "SourceImportOutput.h"

#include <assimp/mesh.h>
#include <assimp/scene.h>

#include <cstdint>

class FbxSkinImporter final
{
  public:
	static ImportedSkeletonIndex ImportSkeleton(
	    const aiScene& scene,
	    const aiNode& meshNode,
	    const aiMesh& mesh,
	    std::uint32_t sourceMeshIndex,
	    SourceImportOutput& output);
	static void ImportSkinInfluences(
	    const aiMesh& mesh,
	    const ImportedSkeleton& skeleton,
	    ImportedMeshGeometry& geometry);

  private:
	struct SkeletonBuildState;
	struct InfluenceBuildState;

	static void CollectSkeletonTopology(
	    const aiScene& scene,
	    const aiNode& meshNode,
	    const aiMesh& mesh,
	    SkeletonBuildState& state);
	static void InitializeSkeleton(
	    const aiScene& scene,
	    const aiMesh& mesh,
	    std::uint32_t sourceMeshIndex,
	    SkeletonBuildState& state);
	static void AppendSkeletonJoints(const aiScene& scene, const aiMesh& mesh, SkeletonBuildState& state);
	static void CollectSkinWeights(
	    const aiMesh& mesh,
	    const ImportedSkeleton& skeleton,
	    InfluenceBuildState& state);
	static void WriteSkinInfluences(
	    const aiMesh& mesh,
	    ImportedMeshGeometry& geometry,
	    const InfluenceBuildState& state);
};
