#include "../../../PCH.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularSettings.h"

#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularCVars.h"
#include "SceneData/MaterialCVars.h"

#include <algorithm>

namespace
{
	constexpr std::uint32_t MaxSupportedBounceCount = 8u;
}

IndirectSpecularSettings BuildIndirectSpecularSettings() noexcept
{
	return IndirectSpecularSettings{
	    .SampleMode = CVarIndirectSpecularSampleMode.Get(),
	    .DebugMode = CVarIndirectSpecularDebugMode.Get(),
	    .MaterialMode = CVarRendererMaterialBindingMode.Get(),
	    .NormalBias = std::max(CVarIndirectSpecularNormalBias.Get(), 0.0f),
	    .MaxDistance = std::max(CVarIndirectSpecularMaxDistance.Get(), 0.001f),
	    .BounceCount = std::clamp(CVarIndirectSpecularBounceCount.Get(), 1u, MaxSupportedBounceCount)};
}
