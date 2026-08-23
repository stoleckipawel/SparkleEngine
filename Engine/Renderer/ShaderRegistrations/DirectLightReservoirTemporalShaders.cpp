#include "PCH.h"

#include "Passes/Lighting/Direct/DirectLightReservoirTemporalShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightReservoirTemporalCS,
    RendererShaderPackages::DirectLightReservoirTemporal,
    "/Engine/Passes/Lighting/Direct/DirectLightReservoirTemporal.hlsl",
    "main",
    Compute);
