#include "../../../PCH.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"

#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingCVars.h"

#include <algorithm>

namespace
{
	constexpr std::uint32_t MaxSupportedSamplesPerPixel = 4096u;
	constexpr std::uint32_t MaxSupportedBounceCount = 16u;
}

PathTracedLightingSettings BuildPathTracedLightingSettings() noexcept
{
	return PathTracedLightingSettings{
	    .SamplesPerPixel = std::clamp(CVarPathTracedLightingSamplesPerPixel.Get(), 1u, MaxSupportedSamplesPerPixel),
	    .BounceCount = std::clamp(CVarPathTracedLightingBounceCount.Get(), 1u, MaxSupportedBounceCount),
	    .NormalBias = std::max(CVarPathTracedLightingNormalBias.Get(), 0.0f),
	    .MaxDistance = std::max(CVarPathTracedLightingMaxDistance.Get(), 0.001f)};
}
