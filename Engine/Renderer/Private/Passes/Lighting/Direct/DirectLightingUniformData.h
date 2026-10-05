#pragma once

#include <cstdint>

struct DirectLightingUniformData final
{
	std::uint32_t EvaluateDiffuse = 1u;
	std::uint32_t EvaluateSpecular = 1u;
	std::uint32_t EvaluateSubsurface = 1u;
	std::uint32_t EvaluateShadows = 1u;
};

static_assert(sizeof(DirectLightingUniformData) == 16u);
