#include "PCH.h"
#include "Mesh.h"

Mesh::Mesh(const Transform& transform) noexcept : m_transform(transform) {}

Mesh::Mesh(const DirectX::XMFLOAT3& translation, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scale) noexcept :
    Mesh(Transform(translation, rotation, scale))
{
}

void Mesh::SetTranslation(const DirectX::XMFLOAT3& t) noexcept
{
	m_transform.SetTranslation(t);
}

void Mesh::SetRotationEuler(const DirectX::XMFLOAT3& r) noexcept
{
	m_transform.SetRotationEuler(r);
}

void Mesh::SetScale(const DirectX::XMFLOAT3& s) noexcept
{
	m_transform.SetScale(s);
}

DirectX::XMMATRIX Mesh::GetWorldMatrix() const noexcept
{
	return m_transform.GetWorldMatrix();
}

DirectX::XMMATRIX Mesh::GetWorldInverseTransposeMatrix() const noexcept
{
	return m_transform.GetWorldInverseTransposeMatrix();
}

DirectX::XMFLOAT3X3 Mesh::GetWorldRotationMatrix3x3() const noexcept
{
	return m_transform.GetRotationMatrix3x3();
}

void Mesh::RebuildGeometry()
{
	m_meshData.Clear();
	GenerateGeometry(m_meshData);
	m_bGeometryDirty = false;
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
