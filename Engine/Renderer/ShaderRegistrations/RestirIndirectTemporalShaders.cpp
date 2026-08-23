#include "PCH.h"

#include "Passes/RayTracing/RestirIndirectTemporalShader.h"

IMPLEMENT_GLOBAL_SHADER(
    RestirIndirectTemporalCS, "/Engine/Passes/RayTracing/RestirIndirectTemporal.hlsl",
    "main",
    Compute);
