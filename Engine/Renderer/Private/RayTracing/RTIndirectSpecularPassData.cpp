#include "../PCH.h"
#include "RayTracing/RTIndirectSpecularPassData.h"

#include "RayTracing/RTIndirectSpecularCVars.h"

#include <algorithm>

namespace RTIndirectSpecularPassData
{
	RTIndirectSpecularUniformData Build() noexcept
	{
		return RTIndirectSpecularUniformData{
		    .DebugMode = static_cast<std::uint32_t>(CVarRTIndirectSpecularDebugMode.Get()),
		    .NormalBias = std::max(CVarRTIndirectSpecularNormalBias.Get(), 0.0f),
		    .MaxDistance = std::max(CVarRTIndirectSpecularMaxDistance.Get(), 0.001f),
		    .Padding0 = 0.0f};
	}
}

