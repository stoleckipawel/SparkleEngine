#pragma once

#include <cstdint>
#include <type_traits>

struct RestirIndirectLightingUniformData
{
	std::uint32_t BounceCount = 1u;
	std::uint32_t TemporalReuse = 1u;
	std::uint32_t SpatialReuse = 1u;
	std::uint32_t EvaluateDiffuse = 1u;
	std::uint32_t EvaluateSpecular = 1u;
	std::uint32_t TraceSecondaryShadows = 1u;
	std::uint32_t WriteReconstructionGuides = 0u;
	std::uint32_t Padding = 0u;
};

static_assert(std::is_standard_layout_v<RestirIndirectLightingUniformData>);
static_assert(std::is_trivially_copyable_v<RestirIndirectLightingUniformData>);
static_assert(sizeof(RestirIndirectLightingUniformData) == 32);
