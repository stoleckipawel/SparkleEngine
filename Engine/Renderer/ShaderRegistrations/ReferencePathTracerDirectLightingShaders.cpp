#include "PCH.h"

#include "Passes/RayTracing/ReferencePathTracerDirectLightingShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ReferencePathTracerDirectLightingCS,
    "/Engine/Passes/RayTracing/ReferencePathTracerDirectLighting.hlsl",
    "main",
    Compute);
