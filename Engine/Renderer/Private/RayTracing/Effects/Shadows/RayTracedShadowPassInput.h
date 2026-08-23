#pragma once

#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"
#include <cstdint>

struct RayTracedShadowPassInput final
{
	RayTracedShadowSettings Settings = {};
	std::uint32_t HitInstanceCount = 0u;
	std::uint32_t HitMaterialCount = 0u;
};
