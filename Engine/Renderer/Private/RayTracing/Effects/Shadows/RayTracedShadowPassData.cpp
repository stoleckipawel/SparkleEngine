#include "PCH.h"

#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"

#include "RayTracing/Effects/Shadows/RayTracedShadowPassInput.h"

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(const RayTracedShadowPassInput& input) noexcept
	{
		if (input.HitInstanceCount == 0u)
		{
			return RayTracedShadowUniformData{};
		}

		return RayTracedShadowUniformData{
		    .DirectionalShadowsEnabled = 1u,
		    .LocalLightShadowsEnabled = 1u,
		    .RayTracingHitInstanceCount = input.HitInstanceCount,
		    .RayTracingHitMaterialCount = input.HitMaterialCount,
		    .NormalBias = input.Settings.NormalBias,
		    .MaxDistance = input.Settings.MaxDistance,
		    .Padding2 = 0.0f,
		    .Padding3 = 0.0f};
	}
}
