#include "PCH.h"

#include "Scene/Meshes/SkeletalCookedMesh.h"
#include "Scene/Meshes/MeshMorphEvaluator.h"

SkeletalCookedMesh::SkeletalCookedMesh(SkeletalMeshData&& meshData, Assets::CookedAssetId assetId, std::span<const float> initialMorphWeights) noexcept :
    m_skeletalData(std::move(meshData)), m_baseGeometry(m_skeletalData.geometry), m_morphWeights(initialMorphWeights.begin(), initialMorphWeights.end()),
    m_assetId(assetId)
{
}

void SkeletalCookedMesh::SetMorphWeights(std::span<const float> weights)
{
	if (m_morphWeights.size() == weights.size() && std::equal(m_morphWeights.begin(), m_morphWeights.end(), weights.begin(), weights.end()))
	{
		return;
	}

	m_morphWeights.assign(weights.begin(), weights.end());
	MarkGeometryDirty();
}

void SkeletalCookedMesh::GenerateGeometry(MeshData& outMeshData) const
{
	outMeshData = m_baseGeometry;
	MeshMorphEvaluator::ApplyWeights(outMeshData, m_skeletalData.morphTargets, m_morphWeights);
}
