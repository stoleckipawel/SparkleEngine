#include "PCH.h"

#include "Scene/MeshComponent.h"
#include "Scene/Mesh.h"

MeshComponent::MeshComponent(std::unique_ptr<Mesh>&& mesh) noexcept : m_mesh(std::move(mesh))
{
}

MeshComponent::MeshComponent(std::unique_ptr<Mesh>&& mesh, const Transform& transform, std::uint32_t materialId) noexcept :
	Component(), m_mesh(std::move(mesh)), m_transform(transform), m_materialId(materialId)
{
}

MeshComponent::~MeshComponent() = default;

DirectX::XMMATRIX MeshComponent::GetWorldMatrix() const noexcept
{
	return m_transform.GetWorldMatrix();
}

DirectX::XMMATRIX MeshComponent::GetWorldInverseTransposeMatrix() const noexcept
{
	const DirectX::XMMATRIX world = GetWorldMatrix();
	return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, world));
}