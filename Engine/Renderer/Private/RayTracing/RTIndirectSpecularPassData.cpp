#include "../PCH.h"
#include "RayTracing/RTIndirectSpecularPassData.h"

#include "RayTracing/RTIndirectSpecularSettings.h"

namespace RTIndirectSpecularPassData
{
	RTIndirectSpecularUniformData Build(
	    const RTIndirectSpecularSettings& settings,
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount,
	    bool materialTextureTableAvailable,
	    std::uint32_t materialTextureTableDescriptorCount,
	    std::uint32_t materialTextureTableCapacity) noexcept
	{
		return RTIndirectSpecularUniformData{
		    .DebugMode = static_cast<std::uint32_t>(settings.DebugMode),
		    .RayTracingHitDataAvailable = hitDataAvailable ? 1u : 0u,
		    .NormalBias = settings.NormalBias,
		    .MaxDistance = settings.MaxDistance,
		    .RayTracingHitInstanceCount = hitInstanceCount,
		    .RayTracingHitMaterialCount = hitMaterialCount,
		    .SampleMode = static_cast<std::uint32_t>(settings.SampleMode),
		    .MaterialTextureTableAvailable = materialTextureTableAvailable ? 1u : 0u,
		    .MaterialTextureTableDescriptorCount = materialTextureTableDescriptorCount,
		    .MaterialTextureTableCapacity = materialTextureTableCapacity,
		    .Padding0 = 0u,
		    .Padding1 = 0u};
	}
}
