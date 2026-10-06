#include "../../../PCH.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"

#include <algorithm>

ConsoleVariable<std::uint32_t> CVarRestirIndirectLightingBounceCount(
    "r.RayTracing.Restir.Indirect.Bounces",
    2u,
    "Maximum path bounce count used to generate ReSTIR indirect candidates.");

ConsoleVariable<bool> CVarRestirIndirectTemporalReuse(
    "r.RayTracing.Restir.Indirect.TemporalReuse",
    true,
    "Reuse previous-frame ReSTIR indirect candidates.");

ConsoleVariable<bool> CVarRestirIndirectSpatialReuse(
    "r.RayTracing.Restir.Indirect.SpatialReuse",
    true,
    "Reuse neighboring ReSTIR indirect candidates.");

std::uint32_t ResolveRestirIndirectBounceCount() noexcept
{
	return std::clamp(CVarRestirIndirectLightingBounceCount.Get(), 1u, 8u);
}
