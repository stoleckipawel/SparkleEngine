#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

struct alignas(256) ViewTemporalUniformData
{
	DirectX::XMFLOAT4X4 PreviousWorldToViewMatrix = {};
	DirectX::XMFLOAT4X4 PreviousViewToClipMatrix = {};
	DirectX::XMFLOAT4X4 PreviousWorldToClipMatrix = {};
	DirectX::XMFLOAT2 CurrentJitterNdc = {0.0f, 0.0f};
	DirectX::XMFLOAT2 PreviousJitterNdc = {0.0f, 0.0f};
	std::uint32_t HistoryValid = 0u;
};
static_assert(std::is_standard_layout_v<ViewTemporalUniformData>);
static_assert(std::is_trivially_copyable_v<ViewTemporalUniformData>);
static_assert(alignof(ViewTemporalUniformData) >= 256);
static_assert(sizeof(ViewTemporalUniformData) % 256 == 0);
static_assert(sizeof(ViewTemporalUniformData) <= 64 * 1024);
static_assert(sizeof(ViewTemporalUniformData) == 256u);
static_assert(offsetof(ViewTemporalUniformData, PreviousWorldToViewMatrix) == 0u);
static_assert(offsetof(ViewTemporalUniformData, PreviousViewToClipMatrix) == 64u);
static_assert(offsetof(ViewTemporalUniformData, PreviousWorldToClipMatrix) == 128u);
static_assert(offsetof(ViewTemporalUniformData, CurrentJitterNdc) == 192u);
static_assert(offsetof(ViewTemporalUniformData, PreviousJitterNdc) == 200u);
static_assert(offsetof(ViewTemporalUniformData, HistoryValid) == 208u);
