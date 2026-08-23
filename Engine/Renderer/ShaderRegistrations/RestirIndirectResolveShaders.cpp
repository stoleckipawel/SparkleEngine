#include "PCH.h"

#include "Passes/RayTracing/RestirIndirectResolveShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RestirIndirectResolveCS,
    RendererShaderPackages::RestirIndirectResolve,
    "/Engine/Passes/RayTracing/RestirIndirectResolve.hlsl",
    "main",
    Compute);
