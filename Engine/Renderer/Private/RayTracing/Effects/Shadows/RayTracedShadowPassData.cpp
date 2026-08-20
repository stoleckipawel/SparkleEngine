#include "PCH.h"

#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassInput.h"

static const auto g_rayTracedShadowPassDataLogger = Logging::GetOrCreateLogger("Renderer.RayTracedShadowPassData");

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(
	    const RayTracedShadowPassInput& input,
	    bool hasTraceableInstances,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept
	{
		if (!input.Enabled || !hasTraceableInstances)
		{
			return RayTracedShadowUniformData{};
		}

		if (input.SceneTlasGpuAddress == 0u)
		{
			Diagnostics::Fatal(g_rayTracedShadowPassDataLogger, __FILE__, __LINE__, "Ray-traced shadow pass has no SceneTlas GPU address.");
		}
		return RayTracedShadowUniformData{
		    .DirectionalShadowsEnabled = 1u,
		    .LocalLightShadowsEnabled = 1u,
		    .Padding0 = 0u,
		    .Padding1 = 0u,
		    .NormalBias = input.Settings.NormalBias,
		    .MaxDistance = input.Settings.MaxDistance,
		    .Padding2 = 0.0f,
		    .Padding3 = 0.0f,
		    .SceneTlasGpuAddressLow = static_cast<std::uint32_t>(input.SceneTlasGpuAddress & 0xFFFFFFFFull),
		    .SceneTlasGpuAddressHigh = static_cast<std::uint32_t>((input.SceneTlasGpuAddress >> 32u) & 0xFFFFFFFFull),
		    .RayTracingHitInstanceCount = hitInstanceCount,
		    .RayTracingHitMaterialCount = hitMaterialCount,
		    .Padding4 = 0u,
		    .Padding5 = 0u,
		    .Padding6 = 0u,
		    .Padding7 = 0u};
	}
}
