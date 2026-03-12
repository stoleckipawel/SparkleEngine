#include "PCH.h"
#include "ImportedMesh.h"

ImportedMesh::ImportedMesh(MeshData&& meshData, const DirectX::XMFLOAT4X4& worldTransform) noexcept :
    Mesh(), m_importedData(std::move(meshData)), m_worldTransform(worldTransform)
{
}

DirectX::XMMATRIX ImportedMesh::GetWorldMatrix() const noexcept
{
	return DirectX::XMLoadFloat4x4(&m_worldTransform);
}

DirectX::XMMATRIX ImportedMesh::GetWorldInverseTransposeMatrix() const noexcept
{
	const DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&m_worldTransform);
	return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, world));
}

void ImportedMesh::GenerateGeometry(MeshData& outMeshData) const
{
	outMeshData.vertices = m_importedData.vertices;
	outMeshData.indices = m_importedData.indices;
}
