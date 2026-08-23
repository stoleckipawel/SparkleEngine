#include "PCH.h"

#include "Passes/PostProcessing/ExposureShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureCS,
    RendererShaderPackages::Exposure,
    "/Engine/Passes/PostProcessing/Exposure.hlsl",
    "main",
    Compute);
