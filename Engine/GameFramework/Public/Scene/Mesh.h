#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "MeshData.h"

#include <DirectXMath.h>

class SPARKLE_ENGINE_API Mesh
{
  public:
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
	DirectX::XMFLOAT3 GetTranslation() const noexcept { return m_translation; }

	void SetRotationEuler(const DirectX::XMFLOAT3& r) noexcept;
	DirectX::XMFLOAT3 GetRotationEuler() const noexcept { return m_rotationEuler; }

	void SetScale(const DirectX::XMFLOAT3& s) noexcept;
	DirectX::XMFLOAT3 GetScale() const noexcept { return m_scale; }

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

	void InvalidateWorldCache() noexcept { m_bWorldDirty = true; }

  private:
	void RebuildWorldIfNeeded() const noexcept;

	DirectX::XMFLOAT3 m_translation{0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 m_rotationEuler{0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 m_scale{1.0f, 1.0f, 1.0f};

	mutable DirectX::XMFLOAT4X4 m_worldMatrixCache{};
	mutable bool m_bWorldDirty = true;

	mutable MeshData m_meshData;
	mutable bool m_bGeometryDirty = true;

	uint32 m_materialId = 0;
};
