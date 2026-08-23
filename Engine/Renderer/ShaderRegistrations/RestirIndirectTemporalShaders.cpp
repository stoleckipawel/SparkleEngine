#include "PCH.h"

#include "Passes/RayTracing/RestirIndirectTemporalShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RestirIndirectTemporalCS,
    RendererShaderPackages::RestirIndirectTemporal,
    "/Engine/Passes/RayTracing/RestirIndirectTemporal.hlsl",
    "main",
    Compute);
