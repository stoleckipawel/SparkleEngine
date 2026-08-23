#include "PCH.h"

#include "Passes/RayTracing/PathTracedIndirectLightingShader.h"

IMPLEMENT_GLOBAL_SHADER(
    PathTracedIndirectLightingCS, "/Engine/Passes/RayTracing/PathTracedIndirectLighting.hlsl",
    "main",
    Compute);
