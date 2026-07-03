#include "../../../PCH.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"

#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"

RayTracedShadowSettings BuildRayTracedShadowSettingsFromCVars() noexcept
{
	return RayTracedShadowSettings{
	    .NormalBias = CVarRayTracedShadowNormalBias.Get(),
	    .MaxDistance = CVarRayTracedShadowMaxDistance.Get(),
	    .Enabled = CVarRayTracedShadowsEnabled.Get()};
}
