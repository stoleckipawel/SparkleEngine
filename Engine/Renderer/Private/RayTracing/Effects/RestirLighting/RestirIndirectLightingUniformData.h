#pragma once

#include <cstdint>
#include <type_traits>

struct RestirIndirectLightingUniformData
{
	std::uint32_t BounceCount = 1u;
	std::uint32_t Padding[3] = {};
};

static_assert(std::is_standard_layout_v<RestirIndirectLightingUniformData>);
static_assert(std::is_trivially_copyable_v<RestirIndirectLightingUniformData>);
static_assert(sizeof(RestirIndirectLightingUniformData) == 16);
