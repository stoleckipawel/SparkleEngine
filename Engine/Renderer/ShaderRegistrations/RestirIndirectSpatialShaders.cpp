#include "PCH.h"

#include "Passes/RayTracing/RestirIndirectSpatialShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RestirIndirectSpatialCS,
    RendererShaderPackages::RestirIndirectSpatial,
    "/Engine/Passes/RayTracing/RestirIndirectSpatial.hlsl",
    "main",
    Compute);
