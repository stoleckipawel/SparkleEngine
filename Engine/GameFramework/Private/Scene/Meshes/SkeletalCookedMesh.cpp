#include "PCH.h"

#include "Scene/Meshes/SkeletalCookedMesh.h"

SkeletalCookedMesh::SkeletalCookedMesh(
    SkeletalMeshData&& meshData,
    Assets::CookedAssetId assetId) noexcept :
	Mesh(meshData.geometry),
	m_skeletalData(std::move(meshData)),
	m_assetId(assetId)
{
}

SkeletalCookedMesh::~SkeletalCookedMesh() = default;

SkeletalCookedMesh::SkeletalCookedMesh(
    SkeletalCookedMesh&&) noexcept = default;

SkeletalCookedMesh& SkeletalCookedMesh::operator=(
    SkeletalCookedMesh&&) noexcept = default;
