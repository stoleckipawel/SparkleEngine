#include "PCH.h"
#include "Scene/Meshes/CookedMesh.h"

CookedMesh::CookedMesh(MeshData&& meshData) noexcept :
	Mesh(std::move(meshData))
{
}

CookedMesh::CookedMesh(MeshData&& meshData, Assets::CookedAssetId assetId) noexcept :
	Mesh(std::move(meshData)),
	m_assetId(assetId)
{
}

CookedMesh::~CookedMesh() = default;

CookedMesh::CookedMesh(CookedMesh&&) noexcept = default;

CookedMesh& CookedMesh::operator=(CookedMesh&&) noexcept = default;
