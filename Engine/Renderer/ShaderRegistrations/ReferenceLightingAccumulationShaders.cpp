#include "PCH.h"

#include "Passes/RayTracing/ReferenceLightingAccumulationShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ReferenceLightingAccumulationCS, "/Engine/Passes/RayTracing/ReferenceLightingAccumulation.hlsl",
    "main",
    Compute);
