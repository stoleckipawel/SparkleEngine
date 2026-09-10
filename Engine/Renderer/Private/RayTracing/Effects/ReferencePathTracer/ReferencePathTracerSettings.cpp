#include "../../../PCH.h"
#include "RayTracing/Effects/ReferencePathTracer/ReferencePathTracerSettings.h"

#include "RayTracing/Effects/ReferencePathTracer/ReferencePathTracerCVars.h"

#include <algorithm>

class ReferencePathTracerSettingsConstants final
{
public:
	static constexpr std::uint32_t MaxSupportedSamplesPerPixel = 4096u;
	static constexpr std::uint32_t MaxSupportedBounceCount = 16u;
};

ReferencePathTracerSettings BuildReferencePathTracerSettings() noexcept
{
	return ReferencePathTracerSettings{
	    .SamplesPerPixel =
	        std::clamp(CVarReferencePathTracerSamplesPerPixel.Get(), 1u, ReferencePathTracerSettingsConstants::MaxSupportedSamplesPerPixel),
	    .BounceCount =
	        std::clamp(CVarReferencePathTracerBounceCount.Get(), 1u, ReferencePathTracerSettingsConstants::MaxSupportedBounceCount),
	    .NormalBias = std::max(CVarReferencePathTracerNormalBias.Get(), 0.0f),
	    .MaxDistance = std::max(CVarReferencePathTracerMaxDistance.Get(), 0.001f)};
}
