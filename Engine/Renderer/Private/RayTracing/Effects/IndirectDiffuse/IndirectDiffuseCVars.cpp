#include "../../../PCH.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseCVars.h"

ConsoleVariable<bool> CVarIndirectDiffuseEnabled(
    "r.RayTracing.IndirectDiffuse.Enabled",
    false,
    "Enable full-resolution ray-traced indirect diffuse.");
ConsoleVariable<IndirectDiffuseDebugMode> CVarIndirectDiffuseDebugMode(
    "r.RayTracing.IndirectDiffuse.DebugMode",
    IndirectDiffuseDebugMode::Off,
    "Indirect diffuse debug mode: 0=Off, 1=HitMask, 2=HitDistance, 3=SampleDirection, 4=SamplePdf, 5=HitRadiance, 6=FinalContribution, 7=HitNormal, 8=MaterialBaseColor, 9=MissSkyRadiance, 10=RejectionReason.");
ConsoleVariable<float> CVarIndirectDiffuseNormalBias(
    "r.RayTracing.IndirectDiffuse.NormalBias",
    0.01f,
    "World-space normal offset used by ray-traced indirect diffuse rays.");
ConsoleVariable<float> CVarIndirectDiffuseMaxDistance(
    "r.RayTracing.IndirectDiffuse.MaxDistance",
    100000.0f,
    "Maximum ray distance for ray-traced indirect diffuse rays.");
ConsoleVariable<float> CVarIndirectDiffuseIntensity(
    "r.RayTracing.IndirectDiffuse.Intensity",
    1.0f,
    "Intensity multiplier for ray-traced indirect diffuse.");
