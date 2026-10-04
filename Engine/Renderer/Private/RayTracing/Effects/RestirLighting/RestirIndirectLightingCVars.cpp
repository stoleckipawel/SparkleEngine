#include "../../../PCH.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"

ConsoleVariable<std::uint32_t> CVarRestirIndirectLightingBounceCount(
    "r.RayTracing.Restir.Indirect.Bounces",
    2u,
    "Maximum path bounce count used to generate ReSTIR indirect candidates.");

ConsoleVariable<bool> CVarRestirIndirectTemporalReuse(
    "r.RayTracing.Restir.Indirect.TemporalReuse", true, "Reuse previous-frame ReSTIR indirect candidates.");

ConsoleVariable<bool> CVarRestirIndirectSpatialReuse(
    "r.RayTracing.Restir.Indirect.SpatialReuse", true, "Reuse neighboring ReSTIR indirect candidates.");
