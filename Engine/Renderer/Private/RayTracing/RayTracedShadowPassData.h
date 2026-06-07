#pragma once

#include "RayTracing/RayTracedShadowUniformData.h"

struct RenderRayTracingPassServices;

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(const RenderRayTracingPassServices* services, bool hasSceneTlas) noexcept;
}
