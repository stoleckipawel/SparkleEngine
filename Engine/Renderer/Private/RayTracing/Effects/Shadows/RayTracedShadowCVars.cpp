#include "../../../PCH.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"

ConsoleVariable<float> CVarRayTracedShadowNormalBias(
    "r.RayTracedShadows.NormalBias",
    0.01f,
    "World-space normal offset used by ray traced shadow rays.");
ConsoleVariable<float> CVarRayTracedShadowMaxDistance(
    "r.RayTracedShadows.MaxDistance",
    100000.0f,
    "Maximum ray distance for directional ray traced shadows.");
