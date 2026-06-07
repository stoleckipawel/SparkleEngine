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
		    .DirectionalShadowsEnabled = 1u,
		    .LocalLightShadowsEnabled = 1u,
		    .DiagnosticsEnabled = settings->DiagnosticsEnabled ? 1u : 0u,
		    .QualityMode = static_cast<std::uint32_t>(settings->QualityMode),
		    .NormalBias = settings->NormalBias,
		    .MaxDistance = settings->MaxDistance,
		    .Padding0 = 0.0f,
		    .Padding1 = 0.0f};
	}
}
