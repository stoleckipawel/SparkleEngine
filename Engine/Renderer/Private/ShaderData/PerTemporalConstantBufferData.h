#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

struct alignas(256) PerTemporalConstantBufferData
{
	DirectX::XMFLOAT4X4 PreviousWorldToViewMatrix = {};
	DirectX::XMFLOAT4X4 PreviousViewToClipMatrix = {};
	DirectX::XMFLOAT4X4 PreviousWorldToClipMatrix = {};
	DirectX::XMFLOAT2 CurrentJitterNdc = {0.0f, 0.0f};
	DirectX::XMFLOAT2 PreviousJitterNdc = {0.0f, 0.0f};
	uint32_t HistoryValid = 0;
	DirectX::XMFLOAT4 _pad0 = {};
	DirectX::XMFLOAT4 _pad1 = {};
};
static_assert(std::is_standard_layout_v<PerTemporalConstantBufferData>);
static_assert(std::is_trivially_copyable_v<PerTemporalConstantBufferData>);
static_assert(alignof(PerTemporalConstantBufferData) >= 256);
static_assert(sizeof(PerTemporalConstantBufferData) % 256 == 0);
static_assert(sizeof(PerTemporalConstantBufferData) <= 64 * 1024);
