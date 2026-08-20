#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

struct alignas(256) ViewUniformData
{
	DirectX::XMFLOAT2 ViewportSize = {1.0f, 1.0f};
	DirectX::XMFLOAT2 ViewportSizeInv = {1.0f, 1.0f};
	std::uint32_t ViewModeIndex = 0u;
};
static_assert(std::is_standard_layout_v<ViewUniformData>);
static_assert(std::is_trivially_copyable_v<ViewUniformData>);
static_assert(alignof(ViewUniformData) >= 256);
static_assert(sizeof(ViewUniformData) % 256 == 0);
static_assert(sizeof(ViewUniformData) <= 64 * 1024);
static_assert(offsetof(ViewUniformData, ViewportSize) == 0u);
static_assert(offsetof(ViewUniformData, ViewportSizeInv) == 8u);
static_assert(offsetof(ViewUniformData, ViewModeIndex) == 16u);
