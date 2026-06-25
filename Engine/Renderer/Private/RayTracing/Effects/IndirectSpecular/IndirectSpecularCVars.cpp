#include "../../../PCH.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularCVars.h"

ConsoleVariable<bool> CVarIndirectSpecularEnabled(
    "r.RayTracing.Reflections.Enabled",
    false,
    "Enable full-resolution ray-traced indirect specular.");
ConsoleVariable<IndirectSpecularDebugMode> CVarIndirectSpecularDebugMode(
    "r.RayTracing.Reflections.DebugMode",
    IndirectSpecularDebugMode::Off,
    "Indirect specular debug mode. Reflection: 3=MirrorDirection, 10=SampleDirection, 11=SamplePdf, 12=SampleThroughput, 13=HitRadiance, 14=FinalContribution. Shared ray-hit/material: 0=Off, 1=HitMask, 2=HitDistance, 4=HitUV, 5=HitNormal, 6=MaterialId, 7=GeometryClass, 8=HitRejectionReason, 15=MaterialBaseColor, 16=MaterialRoughnessMetallic, 17=MaterialEmissive, 20=HitTangent, 21=HitBitangent, 22=HitNormalTangent, 23=HitSampledNormal, 24=AlphaAcceptedRejected, 25=AlphaSample, 26=AlphaCutoff.");
ConsoleVariable<IndirectSpecularSampleMode> CVarIndirectSpecularSampleMode(
    "r.RayTracing.Reflections.SampleMode",
    IndirectSpecularSampleMode::StochasticGGX,
    "Indirect specular sample mode: 0=Mirror, 1=StochasticGGX.");
ConsoleVariable<float> CVarIndirectSpecularNormalBias(
    "r.RayTracing.Reflections.NormalBias",
    0.01f,
    "World-space normal offset used by ray-traced indirect specular rays.");
ConsoleVariable<float> CVarIndirectSpecularMaxDistance(
    "r.RayTracing.Reflections.MaxDistance",
    100000.0f,
    "Maximum ray distance for ray-traced indirect specular rays.");
ConsoleVariable<std::uint32_t> CVarIndirectSpecularBounceCount(
    "r.RayTracing.Reflections.Bounces",
    1u,
    "Maximum reflection path bounce count for ray-traced indirect specular.");
