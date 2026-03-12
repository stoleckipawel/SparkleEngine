#include "PCH.h"
#include "Mesh.h"

Mesh::Mesh(const DirectX::XMFLOAT3& translation, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scale) noexcept :
    m_translation(translation), m_rotationEuler(rotation), m_scale(scale)
{
}

void Mesh::SetTranslation(const DirectX::XMFLOAT3& t) noexcept
{
	m_translation = t;
	InvalidateWorldCache();
}

void Mesh::SetRotationEuler(const DirectX::XMFLOAT3& r) noexcept
{
	m_rotationEuler = r;
	InvalidateWorldCache();
}

void Mesh::SetScale(const DirectX::XMFLOAT3& s) noexcept
{
	m_scale = s;
	InvalidateWorldCache();
}

void Mesh::RebuildWorldIfNeeded() const noexcept
{
	if (!m_bWorldDirty)
		return;

	const DirectX::XMMATRIX S = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	const DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(m_rotationEuler.x, m_rotationEuler.y, m_rotationEuler.z);
	const DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(m_translation.x, m_translation.y, m_translation.z);
	DirectX::XMStoreFloat4x4(&m_worldMatrixCache, S * R * T);
	m_bWorldDirty = false;
}

DirectX::XMMATRIX Mesh::GetWorldMatrix() const noexcept
{
	RebuildWorldIfNeeded();
	return DirectX::XMLoadFloat4x4(&m_worldMatrixCache);
}

DirectX::XMMATRIX Mesh::GetWorldInverseTransposeMatrix() const noexcept
{
	RebuildWorldIfNeeded();
	const DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&m_worldMatrixCache);
	return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, world));
}

DirectX::XMFLOAT3X3 Mesh::GetWorldRotationMatrix3x3() const noexcept
{
	using namespace DirectX;
	const XMMATRIX R = XMMatrixRotationRollPitchYaw(m_rotationEuler.x, m_rotationEuler.y, m_rotationEuler.z);
	XMFLOAT3X3 rot3x3;
	XMStoreFloat3x3(&rot3x3, R);
	return rot3x3;
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
