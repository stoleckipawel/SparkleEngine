#include "../../../PCH.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"

ConsoleVariable<std::uint32_t> CVarRestirIndirectLightingBounceCount(
    "r.RayTracing.Restir.Indirect.Bounces",
    2u,
    "Maximum path bounce count used to generate ReSTIR indirect candidates.");
ConsoleVariable<float> CVarRestirIndirectLightingNormalBias(
    "r.RayTracing.Restir.Indirect.NormalBias",
    0.01f,
    "World-space normal offset for ReSTIR indirect candidate rays.");
ConsoleVariable<float> CVarRestirIndirectLightingMaxDistance(
    "r.RayTracing.Restir.Indirect.MaxDistance",
    100000.0f,
    "Maximum distance for ReSTIR indirect candidate rays.");
