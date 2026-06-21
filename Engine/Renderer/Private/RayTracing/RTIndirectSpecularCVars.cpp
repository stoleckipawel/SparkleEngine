#include "../PCH.h"
#include "RayTracing/RTIndirectSpecularCVars.h"

ConsoleVariable<bool> CVarRTIndirectSpecularEnabled(
    "r.RayTracing.Reflections.Enabled",
    false,
    "Enable full-resolution RT indirect specular ray queries.");
ConsoleVariable<RTIndirectSpecularDebugMode> CVarRTIndirectSpecularDebugMode(
    "r.RayTracing.Reflections.DebugMode",
    RTIndirectSpecularDebugMode::Off,
    "RT reflection debug mode. Reflection: 3=MirrorDirection, 10=SampleDirection, 11=SamplePdf, 12=SampleThroughput, 13=HitRadiance, 14=FinalContribution. Shared ray-hit/material: 0=Off, 1=HitMask, 2=HitDistance, 4=HitUV, 5=HitNormal, 6=MaterialId, 7=GeometryClass, 8=HitRejectionReason, 15=MaterialBaseColor, 16=MaterialRoughnessMetallic, 17=MaterialEmissive, 20=HitTangent, 21=HitBitangent, 22=HitNormalTangent, 23=HitSampledNormal, 24=AlphaAcceptedRejected, 25=AlphaSample, 26=AlphaCutoff.");
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
