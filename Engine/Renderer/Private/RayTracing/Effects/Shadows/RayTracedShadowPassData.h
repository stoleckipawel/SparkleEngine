#pragma once

#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"

#include <cstdint>

struct RayTracedShadowPassInput;

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(
	    const RayTracedShadowPassInput& input,
	    bool hasTraceableInstances,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept;
}
