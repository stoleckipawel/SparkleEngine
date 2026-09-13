#include "../../../PCH.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"

#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"

#include <algorithm>

RestirIndirectLightingSettings BuildRestirIndirectLightingSettings() noexcept
{
	return RestirIndirectLightingSettings{
	    .BounceCount = std::clamp(CVarRestirIndirectLightingBounceCount.Get(), 1u, 8u)};
}
