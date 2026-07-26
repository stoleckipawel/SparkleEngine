#include "PCH.h"

#include "Scene/Meshes/SkeletalCookedMesh.h"

SkeletalCookedMesh::SkeletalCookedMesh(
    SkeletalMeshData&& meshData,
    Assets::CookedAssetId assetId) noexcept :
	m_skeletalData(std::move(meshData)),
	m_baseGeometry(m_skeletalData.geometry),
	m_assetId(assetId)
{
}

SkeletalCookedMesh::~SkeletalCookedMesh() = default;

SkeletalCookedMesh::SkeletalCookedMesh(
    SkeletalCookedMesh&&) noexcept = default;

SkeletalCookedMesh& SkeletalCookedMesh::operator=(
    SkeletalCookedMesh&&) noexcept = default;

void SkeletalCookedMesh::GenerateGeometry(MeshData& outMeshData) const
{
	outMeshData = m_baseGeometry;
}
