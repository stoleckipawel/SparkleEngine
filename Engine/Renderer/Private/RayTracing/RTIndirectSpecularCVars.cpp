#include "../PCH.h"
#include "RayTracing/RTIndirectSpecularCVars.h"

ConsoleVariable<bool> CVarRTIndirectSpecularEnabled(
    "r.RayTracing.Reflections.Enabled",
    false,
    "Enable full-resolution RT indirect specular ray queries.");
ConsoleVariable<RTIndirectSpecularDebugMode> CVarRTIndirectSpecularDebugMode(
    "r.RayTracing.Reflections.DebugMode",
    RTIndirectSpecularDebugMode::Off,
    "RT indirect specular debug mode: 0=Off, 1=HitMask, 2=HitDistance, 3=MirrorDirection, 4=HitUV, 5=HitNormal, 6=MaterialId, 7=GeometryClass, 8=FallbackReason, 9=AlphaPolicy, 10=SampleDirection, 11=SamplePdf, 12=SampleThroughput, 13=HitRadiance, 14=FinalContribution.");
ConsoleVariable<RTIndirectSpecularSampleMode> CVarRTIndirectSpecularSampleMode(
    "r.RayTracing.Reflections.SampleMode",
    RTIndirectSpecularSampleMode::StochasticGGX,
    "RT indirect specular sample mode: 0=Mirror, 1=StochasticGGX.");
ConsoleVariable<float> CVarRTIndirectSpecularNormalBias(
    "r.RayTracing.Reflections.NormalBias",
    0.01f,
    "World-space normal offset used by RT indirect specular rays.");
ConsoleVariable<float> CVarRTIndirectSpecularMaxDistance(
    "r.RayTracing.Reflections.MaxDistance",
    100000.0f,
    "Maximum ray distance for RT indirect specular rays.");
