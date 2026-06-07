#include "PCH.h"

#include "Scene/Meshes/SkeletalCookedMesh.h"

SkeletalCookedMesh::SkeletalCookedMesh(SkeletalMeshData&& meshData, Assets::CookedAssetId assetId) noexcept :
    m_skeletalData(std::move(meshData)), m_assetId(assetId)
{
}

void SkeletalCookedMesh::GenerateGeometry(MeshData& outMeshData) const
{
	outMeshData = m_skeletalData.geometry;
}
