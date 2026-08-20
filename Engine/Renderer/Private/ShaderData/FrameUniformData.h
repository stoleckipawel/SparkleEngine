#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

struct alignas(256) FrameUniformData
{
	std::uint32_t FrameIndex = 0u;
	float TotalTimeSeconds = 0.0f;
	float DeltaTimeSeconds = 0.0f;
	float ScaledTotalTimeSeconds = 0.0f;
	float ScaledDeltaTimeSeconds = 0.0f;
};
static_assert(std::is_standard_layout_v<FrameUniformData>);
static_assert(std::is_trivially_copyable_v<FrameUniformData>);
static_assert(alignof(FrameUniformData) >= 256);
static_assert(sizeof(FrameUniformData) % 256 == 0);
static_assert(sizeof(FrameUniformData) <= 64 * 1024);
static_assert(offsetof(FrameUniformData, FrameIndex) == 0u);
static_assert(offsetof(FrameUniformData, TotalTimeSeconds) == 4u);
static_assert(offsetof(FrameUniformData, DeltaTimeSeconds) == 8u);
static_assert(offsetof(FrameUniformData, ScaledTotalTimeSeconds) == 12u);
static_assert(offsetof(FrameUniformData, ScaledDeltaTimeSeconds) == 16u);
