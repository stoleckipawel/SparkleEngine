#include "PCH.h"
#include "Scene/Meshes/CookedMesh.h"

CookedMesh::CookedMesh(MeshData&& meshData) noexcept : m_cookedData(std::move(meshData)) {}

void CookedMesh::GenerateGeometry(MeshData& outMeshData) const
{
	outMeshData.vertices = m_cookedData.vertices;
	outMeshData.indices = m_cookedData.indices;
}