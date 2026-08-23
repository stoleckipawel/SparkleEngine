#include "PCH.h"

#include "Passes/PostProcessing/ExposureReduceTextureShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ExposureReduceTextureCS, "/Engine/Passes/PostProcessing/ExposureReduceTexture.hlsl",
    "main",
    Compute);
