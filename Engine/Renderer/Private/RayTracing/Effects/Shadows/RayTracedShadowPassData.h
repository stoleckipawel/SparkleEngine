#pragma once

#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"

#include <cstdint>

struct RenderRayTracingPassServices;

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(
	    const RenderRayTracingPassServices* services,
	    bool hasSceneTlas,
	    bool hasAlphaTestResources,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept;
}
