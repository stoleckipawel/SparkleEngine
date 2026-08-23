#include "PCH.h"

#include "Passes/RayTracing/RestirIndirectResolveShader.h"

IMPLEMENT_GLOBAL_SHADER(
    RestirIndirectResolveCS, "/Engine/Passes/RayTracing/RestirIndirectResolve.hlsl",
    "main",
    Compute);
