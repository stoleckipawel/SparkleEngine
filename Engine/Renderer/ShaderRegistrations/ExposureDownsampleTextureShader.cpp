#include "PCH.h"

#include "Passes/PostProcessing/ExposureDownsampleTextureShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ExposureDownsampleTextureCS, "/Engine/Passes/PostProcessing/ExposureDownsampleTexture.hlsl",
    "main",
    Compute);
