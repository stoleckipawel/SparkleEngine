#include "PCH.h"

#include "Passes/RayTracing/ReferencePathTracerIndirectLightingShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ReferencePathTracerIndirectLightingCS,
    "/Engine/Passes/RayTracing/ReferencePathTracerIndirectLighting.hlsl",
    "main",
    Compute);
