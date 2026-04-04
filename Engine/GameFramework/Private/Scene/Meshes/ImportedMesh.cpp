#include "PCH.h"
#include "ImportedMesh.h"

ImportedMesh::ImportedMesh(MeshData&& meshData) noexcept : m_importedData(std::move(meshData)) {}

void ImportedMesh::GenerateGeometry(MeshData& outMeshData) const
{
	outMeshData.vertices = m_importedData.vertices;
	outMeshData.indices = m_importedData.indices;
}
