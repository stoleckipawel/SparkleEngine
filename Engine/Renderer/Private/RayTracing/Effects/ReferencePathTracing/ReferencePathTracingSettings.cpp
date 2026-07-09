#include "../../../PCH.h"
#include "RayTracing/Effects/ReferencePathTracing/ReferencePathTracingSettings.h"

#include "RayTracing/Effects/ReferencePathTracing/ReferencePathTracingCVars.h"

#include <algorithm>

namespace
{
	constexpr std::uint32_t MaxSupportedSamplesPerPixel = 4096u;
	constexpr std::uint32_t MaxSupportedBounceCount = 16u;
}

ReferencePathTracingSettings BuildReferencePathTracingSettings() noexcept
{
	return ReferencePathTracingSettings{
	    .SamplesPerPixel = std::clamp(CVarReferencePathTracingSamplesPerPixel.Get(), 1u, MaxSupportedSamplesPerPixel),
	    .BounceCount = std::clamp(CVarReferencePathTracingBounceCount.Get(), 1u, MaxSupportedBounceCount),
	    .NormalBias = std::max(CVarReferencePathTracingNormalBias.Get(), 0.0f),
	    .MaxDistance = std::max(CVarReferencePathTracingMaxDistance.Get(), 0.001f)};
}
