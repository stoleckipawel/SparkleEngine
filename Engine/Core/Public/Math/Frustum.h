#pragma once

#include "Core/Public/CoreAPI.h"

#include <DirectXMath.h>

#include <array>
#include <cstddef>

struct SPARKLE_CORE_API Frustum
{
	inline static constexpr std::size_t kPlaneCount = 6;

	std::array<DirectX::XMFLOAT4, kPlaneCount> planes;

	void ExtractFromViewProjection(const DirectX::XMFLOAT4X4& viewProj) noexcept;
};
