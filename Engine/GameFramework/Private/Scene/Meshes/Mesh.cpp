#include "PCH.h"
#include "Scene/Meshes/Mesh.h"

void Mesh::RebuildGeometry()
{
	++m_geometryRevision;
	m_meshData.Clear();
	GenerateGeometry(m_meshData);
	m_bGeometryDirty = false;
}

void Mesh::MarkGeometryDirty() noexcept
{
	m_bGeometryDirty = true;
	++m_geometryRevision;
}

const MeshData& Mesh::GetMeshData() const
{
	if (m_bGeometryDirty)
	{
		m_meshData.Clear();
		GenerateGeometry(m_meshData);
		m_bGeometryDirty = false;
	}
	return m_meshData;
}
