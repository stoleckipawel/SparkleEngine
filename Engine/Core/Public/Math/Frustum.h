// ============================================================================
// Frustum.h
// ----------------------------------------------------------------------------
// View frustum representation for culling operations.
//
// USAGE:
//   Frustum frustum;
//   frustum.ExtractFromViewProjection(viewProjMatrix);
//
//   if (frustum.IntersectsSphere(center, radius)) {
//       // Object is potentially visible
//   }
//
// DESIGN:
//   - Six planes representing view frustum boundaries
//   - Planes stored as (A, B, C, D) where Ax + By + Cz + D = 0
//   - Normals point inward (positive half-space is inside frustum)
//   - Supports point and sphere intersection tests
//
// ============================================================================

#pragma once

#include "Core/Public/CoreAPI.h"

#include <DirectXMath.h>

// ============================================================================
// Frustum - Six planes for view frustum culling
// ============================================================================

struct SPARKLE_CORE_API Frustum
{
	enum Plane
	{
		Left = 0,
		Right,
		Bottom,
		Top,
		Near,
		Far,
		Count
	};

	DirectX::XMFLOAT4 planes[Count];

	// ========================================================================
	// Extraction
	// ========================================================================

	/// Extracts frustum planes from a view-projection matrix.
	/// Uses Gribb/Hartmann method for row-major matrices.
	void ExtractFromViewProjection(const DirectX::XMFLOAT4X4& viewProj) noexcept;

	// ========================================================================
	// Intersection Tests
	// ========================================================================

	/// Tests if a point is inside the frustum.
	bool ContainsPoint(const DirectX::XMFLOAT3& point) const noexcept;

	/// Tests if a sphere intersects the frustum.
	bool IntersectsSphere(const DirectX::XMFLOAT3& center, float radius) const noexcept;

	/// Tests if an axis-aligned bounding box intersects the frustum.
	bool IntersectsAABB(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max) const noexcept;
};
