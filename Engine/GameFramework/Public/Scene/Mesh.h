#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Core/Public/Math/Transform.h"
#include "MeshData.h"

#include <DirectXMath.h>

class SPARKLE_ENGINE_API Mesh
{
  public:
	explicit Mesh(const Transform& transform) noexcept;

	Mesh(
	    const DirectX::XMFLOAT3& translation = {0.0f, 0.0f, 0.0f},
	    const DirectX::XMFLOAT3& rotation = {0.0f, 0.0f, 0.0f},
	    const DirectX::XMFLOAT3& scale = {1.0f, 1.0f, 1.0f}) noexcept;

	virtual ~Mesh() = default;
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	void SetTranslation(const DirectX::XMFLOAT3& t) noexcept;
	DirectX::XMFLOAT3 GetTranslation() const noexcept { return m_transform.GetTranslation(); }

	void SetRotationEuler(const DirectX::XMFLOAT3& r) noexcept;
	DirectX::XMFLOAT3 GetRotationEuler() const noexcept { return m_transform.GetRotationEuler(); }

	void SetScale(const DirectX::XMFLOAT3& s) noexcept;
	DirectX::XMFLOAT3 GetScale() const noexcept { return m_transform.GetScale(); }

	void SetTransform(const Transform& transform) noexcept { m_transform = transform; }
	Transform& GetTransform() noexcept { return m_transform; }
	const Transform& GetTransform() const noexcept { return m_transform; }

	virtual DirectX::XMMATRIX GetWorldMatrix() const noexcept;
	virtual DirectX::XMMATRIX GetWorldInverseTransposeMatrix() const noexcept;
	DirectX::XMFLOAT3X3 GetWorldRotationMatrix3x3() const noexcept;

	void RebuildGeometry();
	const MeshData& GetMeshData() const;

	uint32 GetIndexCount() const noexcept { return m_meshData.GetIndexCount(); }

	void SetMaterialId(uint32 id) noexcept { m_materialId = id; }
	uint32 GetMaterialId() const noexcept { return m_materialId; }

  protected:
	virtual void GenerateGeometry(MeshData& outMeshData) const = 0;

  private:
	Transform m_transform;

	mutable MeshData m_meshData;
	mutable bool m_bGeometryDirty = true;

	uint32 m_materialId = 0;
};
