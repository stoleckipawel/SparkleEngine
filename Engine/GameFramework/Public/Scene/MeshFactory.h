#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Core/Public/CoreTypes.h"

#include <DirectXMath.h>
#include <memory>
#include <vector>

class Mesh;

// =============================================================================
// MeshFactory
// =============================================================================

class SPARKLE_ENGINE_API MeshFactory
{
  public:
	// -------------------------------------------------------------------------
	// Shape Types
	// -------------------------------------------------------------------------

	enum class Shape
	{
		Box,
		Plane,
		Sphere,
		Cone,
		Cylinder,
		Torus,
		Capsule,
		Hemisphere,
		Pyramid,
		Disk,
		Octahedron,
		Tetrahedron,
		Icosahedron,
		Dodecahedron,
		Icosphere,
	};

	// -------------------------------------------------------------------------
	// Factory Methods
	// -------------------------------------------------------------------------

	void Rebuild(Shape shape, uint32 count, const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extents, uint32 seed = 0);

	void AppendShape(
	    Shape shape,
	    const DirectX::XMFLOAT3& translation = {0.0f, 0.0f, 0.0f},
	    const DirectX::XMFLOAT3& rotation = {0.0f, 0.0f, 0.0f},
	    const DirectX::XMFLOAT3& scale = {1.0f, 1.0f, 1.0f});

	void AppendShapes(Shape shape, uint32 count, const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extents, uint32 seed = 0);

	// -------------------------------------------------------------------------
	// Management
	// -------------------------------------------------------------------------

	void Clear() noexcept;

	// -------------------------------------------------------------------------
	// Accessors
	// -------------------------------------------------------------------------

	const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const noexcept { return m_meshes; }
	SizeType GetMeshCount() const noexcept { return m_meshes.size(); }

	/// Transfers ownership of all meshes out of the factory.
	std::vector<std::unique_ptr<Mesh>> TakeMeshes() && noexcept { return std::move(m_meshes); }

  private:
	std::vector<std::unique_ptr<Mesh>> m_meshes;
};
