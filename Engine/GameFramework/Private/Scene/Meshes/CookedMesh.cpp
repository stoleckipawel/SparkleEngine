#include "PCH.h"
#include "Scene/Meshes/CookedMesh.h"

CookedMesh::CookedMesh(MeshData&& meshData) noexcept : m_cookedData(std::move(meshData)) {}

CookedMesh::CookedMesh(MeshData&& meshData, Assets::CookedAssetId assetId) noexcept :
	m_cookedData(std::move(meshData)), m_assetId(assetId)
{
}

void CookedMesh::GenerateGeometry(MeshData& outMeshData) const
{
	outMeshData.vertices = m_cookedData.vertices;
	outMeshData.indices = m_cookedData.indices;
}
