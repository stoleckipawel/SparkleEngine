#include "../../../PCH.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingCVars.h"

ConsoleVariable<std::uint32_t> CVarPathTracedLightingSamplesPerPixel(
    "r.RayTracing.PathTracedLighting.SamplesPerPixel",
    64u,
    "Samples per pixel evaluated by path traced lighting.");
ConsoleVariable<std::uint32_t> CVarPathTracedLightingBounceCount(
    "r.RayTracing.PathTracedLighting.Bounces",
    8u,
    "Maximum secondary bounce count for path traced lighting.");
ConsoleVariable<float> CVarPathTracedLightingNormalBias(
    "r.RayTracing.PathTracedLighting.NormalBias",
    0.01f,
    "World-space normal offset used by path traced lighting rays.");
ConsoleVariable<float> CVarPathTracedLightingMaxDistance(
    "r.RayTracing.PathTracedLighting.MaxDistance",
    100000.0f,
    "Maximum secondary ray distance for path traced lighting.");
