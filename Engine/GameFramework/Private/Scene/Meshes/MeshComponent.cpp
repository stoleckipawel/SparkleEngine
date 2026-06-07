#include "PCH.h"

#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/Mesh.h"

MeshComponent::MeshComponent(std::unique_ptr<Mesh>&& mesh) noexcept : m_mesh(std::move(mesh)) {}

MeshComponent::MeshComponent(
	std::unique_ptr<Mesh>&& mesh,
	SceneMeshKind meshKind,
	const Transform& transform,
	MaterialHandle materialHandle,
	Assets::CookedAssetId meshAssetId,
	SceneMeshAssetIndex meshAssetIndex,
	SceneMeshInstanceGroupIndex meshInstanceGroupIndex,
	Assets::CookedAssetId skeletonAssetId) noexcept :
	Component(),
	m_mesh(std::move(mesh)),
	m_transform(transform),
	m_materialHandle(materialHandle),
	m_meshAssetId(meshAssetId),
	m_meshAssetIndex(meshAssetIndex),
	m_meshInstanceGroupIndex(meshInstanceGroupIndex),
	m_skeletonAssetId(skeletonAssetId),
	m_kind(meshKind)
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
