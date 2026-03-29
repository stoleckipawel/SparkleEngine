#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Scene/TransformComponent.h"
#include "MeshData.h"

#include <DirectXMath.h>

class SPARKLE_ENGINE_API Mesh
{
  public:
	explicit Mesh(const TransformComponent& transform) noexcept;

	Mesh(
	    const DirectX::XMFLOAT3& translation = {0.0f, 0.0f, 0.0f},
	    const DirectX::XMFLOAT3& rotation = {0.0f, 0.0f, 0.0f},
	    const DirectX::XMFLOAT3& scale = {1.0f, 1.0f, 1.0f}) noexcept;

	virtual ~Mesh() = default;
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	void SetTransform(const TransformComponent& transform) noexcept { m_transform = transform; }
	TransformComponent& GetTransform() noexcept { return m_transform; }
	const TransformComponent& GetTransform() const noexcept { return m_transform; }

	virtual DirectX::XMMATRIX GetWorldMatrix() const noexcept;
	virtual DirectX::XMMATRIX GetWorldInverseTransposeMatrix() const noexcept;
	DirectX::XMFLOAT3X3 GetWorldRotationMatrix3x3() const noexcept;

	void RebuildGeometry();
	const MeshData& GetMeshData() const;

	void SetMaterialId(uint32 id) noexcept { m_materialId = id; }
	uint32 GetMaterialId() const noexcept { return m_materialId; }

  protected:
	virtual void GenerateGeometry(MeshData& outMeshData) const = 0;

  private:
	TransformComponent m_transform;

	mutable MeshData m_meshData;
	mutable bool m_bGeometryDirty = true;

	uint32 m_materialId = 0;
};
