#include "PCH.h"

#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "RayTracing/Scene/RayTracingPassContext.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"

static const auto g_rayTracedShadowPassDataLogger = Logging::GetOrCreateLogger("Renderer.RayTracedShadowPassData");

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(
	    const RayTracingPassContext* context,
	    bool hasTraceableInstances,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept
	{
		if (context == nullptr || context->ShadowSettings == nullptr || context->Scene == nullptr)
		{
			Diagnostics::Fatal(
			    g_rayTracedShadowPassDataLogger,
			    __FILE__,
			    __LINE__,
			    "Ray-traced shadow pass context is incomplete.");
		}
		if (!context->ShadowsEnabled || !hasTraceableInstances)
		{
			return RayTracedShadowUniformData{};
		}

		const RayTracedShadowSettings& settings = *context->ShadowSettings;
		const RhiGpuVirtualAddress sceneTlasAddress = context->Scene->GetTlasGpuAddress();
		if (sceneTlasAddress == 0u)
		{
			Diagnostics::Fatal(
			    g_rayTracedShadowPassDataLogger,
			    __FILE__,
			    __LINE__,
			    "Ray-traced shadow pass has no SceneTlas GPU address.");
		}
		return RayTracedShadowUniformData{
		    .DirectionalShadowsEnabled = 1u,
		    .LocalLightShadowsEnabled = 1u,
		    .Padding0 = 0u,
		    .Padding1 = 0u,
		    .NormalBias = settings.NormalBias,
		    .MaxDistance = settings.MaxDistance,
		    .Padding2 = 0.0f,
		    .Padding3 = 0.0f,
		    .SceneTlasGpuAddressLow = static_cast<std::uint32_t>(sceneTlasAddress & 0xFFFFFFFFull),
		    .SceneTlasGpuAddressHigh = static_cast<std::uint32_t>((sceneTlasAddress >> 32u) & 0xFFFFFFFFull),
		    .RayTracingHitInstanceCount = hitInstanceCount,
		    .RayTracingHitMaterialCount = hitMaterialCount,
		    .Padding4 = 0u,
		    .Padding5 = 0u,
		    .Padding6 = 0u,
		    .Padding7 = 0u};
	}
}
