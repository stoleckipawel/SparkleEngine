#include "PCH.h"

#include "RayTracing/RayTracedShadowPassData.h"

#include "RayTracing/RenderRayTracingScene.h"
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

		const RhiGpuVirtualAddress sceneTlasAddress = services != nullptr && services->Scene != nullptr
		                                                 ? services->Scene->GetTlasGpuAddress()
		                                                 : 0u;
		const RayTracingSceneTlasShaderAccessMode accessMode = services != nullptr && services->Scene != nullptr
		                                                           ? services->Scene->GetTlasShaderAccessMode()
		                                                           : RayTracingSceneTlasShaderAccessMode::Descriptor;

		return RayTracedShadowUniformData{
		    .DirectionalShadowsEnabled = settings->Enabled ? 1u : 0u,
		    .LocalLightShadowsEnabled = settings->Enabled ? 1u : 0u,
		    .DiagnosticsEnabled = settings->DiagnosticsEnabled ? 1u : 0u,
		    .QualityMode = static_cast<std::uint32_t>(settings->QualityMode),
		    .NormalBias = settings->NormalBias,
		    .MaxDistance = settings->MaxDistance,
		    .Padding0 = 0.0f,
		    .Padding1 = 0.0f,
		    .SceneTlasGpuAddressLow = static_cast<std::uint32_t>(sceneTlasAddress & 0xFFFFFFFFull),
		    .SceneTlasGpuAddressHigh = static_cast<std::uint32_t>((sceneTlasAddress >> 32u) & 0xFFFFFFFFull),
		    .TlasAccessMode = static_cast<std::uint32_t>(accessMode),
		    .Padding2 = 0u};
	}
}
