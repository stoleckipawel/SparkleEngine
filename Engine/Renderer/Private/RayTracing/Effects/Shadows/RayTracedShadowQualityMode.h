#pragma once

#include <cstdint>

enum class RayTracedShadowQualityMode : std::uint32_t
{
	Hard = 0,
	SoftPunctual,
	Count
};
