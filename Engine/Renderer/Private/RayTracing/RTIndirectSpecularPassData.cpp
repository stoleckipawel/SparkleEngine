#include "../PCH.h"
#include "RayTracing/RTIndirectSpecularPassData.h"

#include "RayTracing/RTIndirectSpecularSettings.h"

namespace RTIndirectSpecularPassData
{
	RTIndirectSpecularUniformData Build(
	    const RTIndirectSpecularSettings& settings,
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept
	{
		return RTIndirectSpecularUniformData{
		    .DebugMode = static_cast<std::uint32_t>(settings.DebugMode),
		    .HitDataAvailable = hitDataAvailable ? 1u : 0u,
		    .NormalBias = settings.NormalBias,
		    .MaxDistance = settings.MaxDistance,
		    .HitInstanceCount = hitInstanceCount,
		    .HitMaterialCount = hitMaterialCount,
		    .SampleMode = static_cast<std::uint32_t>(settings.SampleMode),
		    .Padding0 = 0u};
	}
}
