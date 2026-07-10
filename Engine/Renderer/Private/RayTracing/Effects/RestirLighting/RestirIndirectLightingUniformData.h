#pragma once

#include <cstdint>
#include <type_traits>

struct RestirIndirectLightingUniformData
{
	std::uint32_t BounceCount = 1u;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
	std::uint32_t Padding = 0u;
};

static_assert(std::is_standard_layout_v<RestirIndirectLightingUniformData>);
static_assert(std::is_trivially_copyable_v<RestirIndirectLightingUniformData>);
static_assert(sizeof(RestirIndirectLightingUniformData) == 16);
