#pragma once

#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"

#include <cstdint>

struct RayTracingPassContext;

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(
	    const RayTracingPassContext* context,
	    bool hasTraceableInstances,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept;
}
