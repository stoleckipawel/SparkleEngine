// =============================================================================
// ImportedMesh.h - Mesh loaded from an external file (glTF, FBX, etc.)
// =============================================================================
//
// A Mesh subclass that holds pre-built MeshData and a pre-computed world
// transform from the asset's node hierarchy. Unlike procedural primitives,
// geometry is not generated - it is supplied at construction time.
//
#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Mesh.h"

#include <DirectXMath.h>

// =============================================================================
// ImportedMesh
// =============================================================================

class SPARKLE_ENGINE_API ImportedMesh final : public Mesh
{
  public:
	// -------------------------------------------------------------------------
	// Lifecycle
	// -------------------------------------------------------------------------

	/// Constructs an imported mesh from pre-built data and a world transform.
	ImportedMesh(MeshData&& meshData, const DirectX::XMFLOAT4X4& worldTransform) noexcept;

	~ImportedMesh() override = default;

	ImportedMesh(const ImportedMesh&) = delete;
	ImportedMesh& operator=(const ImportedMesh&) = delete;
	ImportedMesh(ImportedMesh&&) noexcept = default;
	ImportedMesh& operator=(ImportedMesh&&) noexcept = default;

	// -------------------------------------------------------------------------
	// World Matrix Override
	// -------------------------------------------------------------------------

	DirectX::XMMATRIX GetWorldMatrix() const noexcept override;
	DirectX::XMMATRIX GetWorldInverseTransposeMatrix() const noexcept override;

  protected:
	// -------------------------------------------------------------------------
	// NVI: Geometry Generation
	// -------------------------------------------------------------------------

	/// Copies stored mesh data (no procedural generation).
	void GenerateGeometry(MeshData& outMeshData) const override;

  private:
	MeshData m_importedData;
	DirectX::XMFLOAT4X4 m_worldTransform;
};
