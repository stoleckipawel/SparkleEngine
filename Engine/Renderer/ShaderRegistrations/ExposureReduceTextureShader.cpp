#include "PCH.h"

#include "Passes/PostProcessing/ExposureReduceTextureShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureReduceTextureCS,
    RendererShaderPackages::ExposureReduceTexture,
    "/Engine/Passes/PostProcessing/ExposureReduceTexture.hlsl",
    "main",
    Compute);
