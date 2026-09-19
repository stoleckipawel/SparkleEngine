#include "PCH.h"

#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"

#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"

RayTracedShadowUniformData BuildRayTracedShadowUniformData(const PreparedRenderScene& scene) noexcept
{
	const RenderSceneGpuRayTracingBindings& rayTracing = scene.gpuBindings->RayTracing;
	if (rayTracing.InstanceCount == 0u)
	{
		return RayTracedShadowUniformData{};
	}

	return RayTracedShadowUniformData{
	    .DirectionalShadowsEnabled = 1u,
	    .LocalLightShadowsEnabled = 1u,
	    .RayTracingHitInstanceCount = rayTracing.InstanceCount,
	    .RayTracingHitMaterialCount = rayTracing.MaterialCount,
	    .NormalBias = CVarRayTracedShadowNormalBias.Get(),
	    .MaxDistance = CVarRayTracedShadowMaxDistance.Get(),
	    .Padding2 = 0.0f,
	    .Padding3 = 0.0f};
}
