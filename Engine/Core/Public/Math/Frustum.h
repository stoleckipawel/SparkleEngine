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
};
