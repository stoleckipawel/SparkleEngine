#include "../../../PCH.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseSettings.h"

#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseCVars.h"

#include <algorithm>

IndirectDiffuseSettings BuildIndirectDiffuseSettingsFromCVars() noexcept
{
	return IndirectDiffuseSettings{
	    .Enabled = CVarIndirectDiffuseEnabled.Get(),
	    .DebugMode = CVarIndirectDiffuseDebugMode.Get(),
	    .NormalBias = std::max(CVarIndirectDiffuseNormalBias.Get(), 0.0f),
	    .MaxDistance = std::max(CVarIndirectDiffuseMaxDistance.Get(), 0.001f),
	    .Intensity = std::max(CVarIndirectDiffuseIntensity.Get(), 0.0f)};
}
