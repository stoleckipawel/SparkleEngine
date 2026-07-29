#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

struct alignas(256) PerFrameConstantBufferData
{
	uint32_t FrameIndex;
	float TotalTimeSeconds;
	float DeltaTimeSeconds;
	float ScaledTotalTimeSeconds;
	float ScaledDeltaTimeSeconds;
	uint32_t ViewModeIndex;

	DirectX::XMFLOAT2 ViewportSize;
	DirectX::XMFLOAT2 ViewportSizeInv;
};
static_assert(std::is_standard_layout_v<PerFrameConstantBufferData>);
static_assert(std::is_trivially_copyable_v<PerFrameConstantBufferData>);
static_assert(alignof(PerFrameConstantBufferData) >= 256);
static_assert(sizeof(PerFrameConstantBufferData) % 256 == 0);
static_assert(sizeof(PerFrameConstantBufferData) <= 64 * 1024);
