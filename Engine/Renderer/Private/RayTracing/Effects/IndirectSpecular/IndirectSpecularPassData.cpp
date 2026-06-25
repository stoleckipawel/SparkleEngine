#include "../../../PCH.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularPassData.h"

#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularSettings.h"

namespace IndirectSpecularPassData
{
	IndirectSpecularUniformData Build(
	    const IndirectSpecularSettings& settings,
	    const RayTracingPassCapabilities& capabilities,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount,
	    std::uint32_t materialTextureTableCapacity) noexcept
	{
		return IndirectSpecularUniformData{
		    .DebugMode = static_cast<std::uint32_t>(settings.DebugMode),
		    .RayTracingHitDataAvailable = capabilities.HitDataAvailable ? 1u : 0u,
		    .NormalBias = settings.NormalBias,
		    .MaxDistance = settings.MaxDistance,
		    .RayTracingHitInstanceCount = hitInstanceCount,
		    .RayTracingHitMaterialCount = hitMaterialCount,
		    .SampleMode = static_cast<std::uint32_t>(settings.SampleMode),
		    .MaterialTextureTableAvailable = capabilities.MaterialTextureTableAvailable ? 1u : 0u,
		    .MaterialTextureTableDescriptorCount = capabilities.MaterialTextureTableDescriptorCount,
		    .MaterialTextureTableCapacity = materialTextureTableCapacity,
		    .BounceCount = settings.BounceCount,
		    .Padding1 = 0u};
	}
}
