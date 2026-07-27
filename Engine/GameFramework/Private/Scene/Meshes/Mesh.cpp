#include "PCH.h"
#include "Scene/Meshes/Mesh.h"

Mesh::Mesh(MeshData&& meshData) noexcept :
	m_meshData(std::move(meshData))
{
}

Mesh::Mesh(const MeshData& meshData) :
	m_meshData(meshData)
{
}

Mesh::~Mesh() = default;

Mesh::Mesh(Mesh&&) noexcept = default;

Mesh& Mesh::operator=(Mesh&&) noexcept = default;

const MeshData& Mesh::GetMeshData() const noexcept
{
	return m_meshData;
}
