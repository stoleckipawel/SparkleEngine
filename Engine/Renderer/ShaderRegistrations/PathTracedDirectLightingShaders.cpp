#include "PCH.h"

#include "Passes/RayTracing/PathTracedDirectLightingShader.h"

IMPLEMENT_GLOBAL_SHADER(
    PathTracedDirectLightingCS, "/Engine/Passes/RayTracing/PathTracedDirectLighting.hlsl",
    "main",
    Compute);
