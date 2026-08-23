#include "PCH.h"

#include "Passes/PostProcessing/ExposureDownsampleTextureShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureDownsampleTextureCS,
    RendererShaderPackages::ExposureDownsampleTexture,
    "/Engine/Passes/PostProcessing/ExposureDownsampleTexture.hlsl",
    "main",
    Compute);
