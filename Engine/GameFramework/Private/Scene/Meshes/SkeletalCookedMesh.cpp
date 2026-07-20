#include "PCH.h"

#include "Scene/Meshes/SkeletalCookedMesh.h"
#include "Scene/Meshes/MeshMorphEvaluator.h"

SkeletalCookedMesh::SkeletalCookedMesh(SkeletalMeshData&& meshData, Assets::CookedAssetId assetId, std::span<const float> initialMorphWeights) noexcept :
    m_skeletalData(std::move(meshData)), m_baseGeometry(m_skeletalData.geometry), m_assetId(assetId)
{
	MeshMorphEvaluator::ApplyWeights(m_baseGeometry, m_skeletalData.morphTargets, initialMorphWeights);
}

void SkeletalCookedMesh::GenerateGeometry(MeshData& outMeshData) const
{
	outMeshData = m_baseGeometry;
}
