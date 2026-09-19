#include "PCH.h"

#include "Passes/PostProcessing/Exposure/ExposureDownsampleTextureShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ExposureDownsampleTextureCS,
    "/Engine/Passes/PostProcessing/Exposure/ExposureDownsampleTexture.hlsl",
    "main",
    Compute);
