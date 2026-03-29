#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Component.h"
#include "GameFramework/Public/Scene/Transform.h"

#include <DirectXMath.h>
#include <cstdint>
#include <memory>

class Mesh;

class SPARKLE_ENGINE_API MeshComponent final : public Component
{
  public:
	explicit MeshComponent(std::unique_ptr<Mesh>&& mesh) noexcept;
	MeshComponent(std::unique_ptr<Mesh>&& mesh, const Transform& transform, std::uint32_t materialId = 0) noexcept;
	~MeshComponent() override;

	MeshComponent(const MeshComponent&) = delete;
	MeshComponent& operator=(const MeshComponent&) = delete;
	MeshComponent(MeshComponent&&) noexcept = default;
	MeshComponent& operator=(MeshComponent&&) noexcept = default;

	Mesh* GetMesh() noexcept { return m_mesh.get(); }
	const Mesh* GetMesh() const noexcept { return m_mesh.get(); }
	bool HasMesh() const noexcept { return m_mesh != nullptr; }

	void SetTransform(const Transform& transform) noexcept { m_transform = transform; }
	Transform& GetTransform() noexcept { return m_transform; }
	const Transform& GetTransform() const noexcept { return m_transform; }

	void SetMaterialId(std::uint32_t materialId) noexcept { m_materialId = materialId; }
	std::uint32_t GetMaterialId() const noexcept { return m_materialId; }

	DirectX::XMMATRIX GetWorldMatrix() const noexcept;
	DirectX::XMMATRIX GetWorldInverseTransposeMatrix() const noexcept;

  private:
	std::unique_ptr<Mesh> m_mesh;
	Transform m_transform;
	std::uint32_t m_materialId = 0;
};