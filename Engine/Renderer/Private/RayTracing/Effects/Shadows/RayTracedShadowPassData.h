#pragma once

#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"

struct RenderRayTracingPassServices;

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(const RenderRayTracingPassServices* services, bool hasSceneTlas) noexcept;
}
