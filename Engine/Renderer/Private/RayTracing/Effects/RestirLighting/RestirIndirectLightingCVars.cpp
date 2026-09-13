#include "../../../PCH.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"

ConsoleVariable<std::uint32_t> CVarRestirIndirectLightingBounceCount(
    "r.RayTracing.Restir.Indirect.Bounces",
    2u,
    "Maximum path bounce count used to generate ReSTIR indirect candidates.");
