#include "PCH.h"

#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"

#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(
	    const RenderRayTracingPassServices* services,
	    bool hasSceneTlas,
	    bool hasAlphaTestResources,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept
	{
		const RayTracedShadowSettings* settings = services != nullptr ? services->ShadowSettings : nullptr;
		if (settings == nullptr || !hasSceneTlas)
		{
			return RayTracedShadowUniformData{};
		}

		const RhiGpuVirtualAddress sceneTlasAddress = services != nullptr && services->Scene != nullptr
		                                                 ? services->Scene->GetTlasGpuAddress()
		                                                 : 0u;
		return RayTracedShadowUniformData{
		    .DirectionalShadowsEnabled = 1u,
		    .LocalLightShadowsEnabled = 1u,
		    .Padding0 = 0u,
		    .Padding1 = 0u,
		    .NormalBias = settings->NormalBias,
		    .MaxDistance = settings->MaxDistance,
		    .Padding2 = 0.0f,
		    .Padding3 = 0.0f,
		    .SceneTlasGpuAddressLow = static_cast<std::uint32_t>(sceneTlasAddress & 0xFFFFFFFFull),
		    .SceneTlasGpuAddressHigh = static_cast<std::uint32_t>((sceneTlasAddress >> 32u) & 0xFFFFFFFFull),
		    .RayTracingHitDataAvailable = hasAlphaTestResources ? 1u : 0u,
		    .RayTracingHitInstanceCount = hitInstanceCount,
		    .RayTracingHitMaterialCount = hitMaterialCount,
		    .Padding4 = 0u,
		    .Padding5 = 0u,
		    .Padding6 = 0u};
	}
}
