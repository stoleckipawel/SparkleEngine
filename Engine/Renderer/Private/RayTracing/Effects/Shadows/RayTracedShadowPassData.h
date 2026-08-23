#pragma once

#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"

#include <cstdint>

struct RayTracedShadowPassInput;

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(const RayTracedShadowPassInput& input) noexcept;
}
