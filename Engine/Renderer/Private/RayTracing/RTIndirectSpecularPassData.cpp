#include "../PCH.h"
#include "RayTracing/RTIndirectSpecularPassData.h"

#include "RayTracing/RTIndirectSpecularCVars.h"

#include <algorithm>

namespace RTIndirectSpecularPassData
{
	RTIndirectSpecularUniformData Build(
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept
	{
		return RTIndirectSpecularUniformData{
		    .DebugMode = static_cast<std::uint32_t>(CVarRTIndirectSpecularDebugMode.Get()),
		    .HitDataAvailable = hitDataAvailable ? 1u : 0u,
		    .NormalBias = std::max(CVarRTIndirectSpecularNormalBias.Get(), 0.0f),
		    .MaxDistance = std::max(CVarRTIndirectSpecularMaxDistance.Get(), 0.001f),
		    .HitInstanceCount = hitInstanceCount,
		    .HitMaterialCount = hitMaterialCount,
		    .SampleMode = static_cast<std::uint32_t>(CVarRTIndirectSpecularSampleMode.Get()),
		    .Padding0 = 0u};
	}
}
