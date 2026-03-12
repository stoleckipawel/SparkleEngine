#pragma once

#include "Core/Public/CoreAPI.h"

#include <DirectXMath.h>

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

	void ExtractFromViewProjection(const DirectX::XMFLOAT4X4& viewProj) noexcept;

	bool ContainsPoint(const DirectX::XMFLOAT3& point) const noexcept;

	bool IntersectsSphere(const DirectX::XMFLOAT3& center, float radius) const noexcept;

	bool IntersectsAABB(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max) const noexcept;
};
