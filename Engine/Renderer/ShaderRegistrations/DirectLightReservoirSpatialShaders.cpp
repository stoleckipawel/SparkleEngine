#include "PCH.h"

#include "Passes/Lighting/Direct/DirectLightReservoirSpatialShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightReservoirSpatialCS,
    RendererShaderPackages::DirectLightReservoirSpatial,
    "/Engine/Passes/Lighting/Direct/DirectLightReservoirSpatial.hlsl",
    "main",
    Compute);
