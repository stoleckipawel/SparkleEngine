#include "../PCH.h"
#include "RayTracing/RTIndirectSpecularCVars.h"

ConsoleVariable<RTIndirectSpecularDebugMode> CVarRTIndirectSpecularDebugMode(
    "r.RayTracing.Reflections.DebugMode",
    RTIndirectSpecularDebugMode::Off,
    "RT indirect specular debug mode: 0=Off, 1=HitMask, 2=HitDistance, 3=MirrorDirection.");
ConsoleVariable<float> CVarRTIndirectSpecularNormalBias(
    "r.RayTracing.Reflections.NormalBias",
    0.01f,
    "World-space normal offset used by RT indirect specular rays.");
ConsoleVariable<float> CVarRTIndirectSpecularMaxDistance(
    "r.RayTracing.Reflections.MaxDistance",
    100000.0f,
    "Maximum ray distance for RT indirect specular mirror rays.");

