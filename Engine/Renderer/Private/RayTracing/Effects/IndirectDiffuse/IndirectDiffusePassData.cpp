#include "../../../PCH.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffusePassData.h"

#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseSettings.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"

namespace IndirectDiffusePassData
{
	IndirectDiffuseUniformData Build(
	    const IndirectDiffuseSettings& settings,
	    const RayTracingPassCapabilities& capabilities,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount,
	    std::uint32_t materialTextureTableCapacity) noexcept
	{
		return IndirectDiffuseUniformData{
		    .DebugMode = static_cast<std::uint32_t>(settings.DebugMode),
		    .RayTracingHitDataAvailable = capabilities.HitDataAvailable ? 1u : 0u,
		    .NormalBias = settings.NormalBias,
		    .MaxDistance = settings.MaxDistance,
		    .RayTracingHitInstanceCount = hitInstanceCount,
		    .RayTracingHitMaterialCount = hitMaterialCount,
		    .MaterialTextureTableAvailable = capabilities.MaterialTextureTableAvailable ? 1u : 0u,
		    .MaterialTextureTableDescriptorCount = capabilities.MaterialTextureTableDescriptorCount,
		    .MaterialTextureTableCapacity = materialTextureTableCapacity,
		    .Intensity = settings.Intensity,
		    .BounceCount = settings.BounceCount,
		    .Padding1 = 0u};
	}
}
