#include "PCH.h"

#include "RayTracing/RayTracedShadowPassData.h"

#include "RayTracing/RenderRayTracingPassServices.h"
#include "RayTracing/RayTracedShadowSettings.h"

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(const RenderRayTracingPassServices* services, bool hasSceneTlas) noexcept
	{
		const RayTracedShadowSettings* settings = services != nullptr ? services->ShadowSettings : nullptr;
		if (settings == nullptr || !hasSceneTlas)
		{
			return RayTracedShadowUniformData{};
		}

		return RayTracedShadowUniformData{
		    .DirectionalShadowsEnabled = settings->UsesHardShadowVisibility() ? 1u : 0u,
		    .LocalLightShadowsEnabled = settings->UsesHardShadowVisibility() ? 1u : 0u,
		    .DiagnosticsEnabled = settings->DiagnosticsEnabled ? 1u : 0u,
		    .RaysPerPixel = RayTracedShadowSettings::RaysPerPixel,
		    .NormalBias = settings->NormalBias,
		    .MaxDistance = settings->MaxDistance,
		    .Padding0 = 0.0f,
		    .Padding1 = 0.0f};
	}
}
