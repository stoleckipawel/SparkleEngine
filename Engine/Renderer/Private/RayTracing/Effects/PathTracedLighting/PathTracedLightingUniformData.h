#pragma once

#include <cstdint>
#include <type_traits>

struct PathTracedLightingUniformData
{
	std::uint32_t SamplesPerPixel = 1u;
	std::uint32_t BounceCount = 1u;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
};

static_assert(std::is_standard_layout_v<PathTracedLightingUniformData>);
static_assert(std::is_trivially_copyable_v<PathTracedLightingUniformData>);
static_assert(sizeof(PathTracedLightingUniformData) == 16);
