// =============================================================================
// Mesh.h - CPU-side renderable mesh with transform and geometry
// =============================================================================
//
// Base class for primitives and imported meshes. Owns transform (TRS) and
// CPU geometry (MeshData). GPU resources are managed by Renderer's GPUMesh.
//
#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "MeshData.h"

#include <DirectXMath.h>

// =============================================================================
// Mesh
// =============================================================================

class SPARKLE_ENGINE_API Mesh
{
  public:
	// -------------------------------------------------------------------------
	// Lifecycle
	// -------------------------------------------------------------------------

	Mesh(
	    const DirectX::XMFLOAT3& translation = {0.0f, 0.0f, 0.0f},
	    const DirectX::XMFLOAT3& rotation = {0.0f, 0.0f, 0.0f},
	    const DirectX::XMFLOAT3& scale = {1.0f, 1.0f, 1.0f}) noexcept;

	virtual ~Mesh() = default;

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	// -------------------------------------------------------------------------
	// Transform
	// -------------------------------------------------------------------------

	void SetTranslation(const DirectX::XMFLOAT3& t) noexcept;
	DirectX::XMFLOAT3 GetTranslation() const noexcept { return m_translation; }

	void SetRotationEuler(const DirectX::XMFLOAT3& r) noexcept;
	DirectX::XMFLOAT3 GetRotationEuler() const noexcept { return m_rotationEuler; }

	void SetScale(const DirectX::XMFLOAT3& s) noexcept;
	DirectX::XMFLOAT3 GetScale() const noexcept { return m_scale; }

	// -------------------------------------------------------------------------
	// World Matrix
	// -------------------------------------------------------------------------

	virtual DirectX::XMMATRIX GetWorldMatrix() const noexcept;
	virtual DirectX::XMMATRIX GetWorldInverseTransposeMatrix() const noexcept;
	DirectX::XMFLOAT3X3 GetWorldRotationMatrix3x3() const noexcept;

	// -------------------------------------------------------------------------
	// Geometry
	// -------------------------------------------------------------------------

	void RebuildGeometry();

	// Lazily builds geometry on first access.
	const MeshData& GetMeshData() const;

	uint32 GetIndexCount() const noexcept { return m_meshData.GetIndexCount(); }

	// -------------------------------------------------------------------------
	// Material
	// -------------------------------------------------------------------------

	void SetMaterialId(uint32 id) noexcept { m_materialId = id; }
	uint32 GetMaterialId() const noexcept { return m_materialId; }

  protected:
	// -------------------------------------------------------------------------
	// NVI: Geometry Generation
	// -------------------------------------------------------------------------

	virtual void GenerateGeometry(MeshData& outMeshData) const = 0;

	void InvalidateWorldCache() noexcept { m_bWorldDirty = true; }

  private:
	void RebuildWorldIfNeeded() const noexcept;

	// -------------------------------------------------------------------------
	// Transform
	// -------------------------------------------------------------------------

	DirectX::XMFLOAT3 m_translation{0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 m_rotationEuler{0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 m_scale{1.0f, 1.0f, 1.0f};

	mutable DirectX::XMFLOAT4X4 m_worldMatrixCache{};
	mutable bool m_bWorldDirty = true;

	// -------------------------------------------------------------------------
	// Geometry
	// -------------------------------------------------------------------------

	mutable MeshData m_meshData;
	mutable bool m_bGeometryDirty = true;

	// -------------------------------------------------------------------------
	// Material
	// -------------------------------------------------------------------------

	uint32 m_materialId = 0;
};
