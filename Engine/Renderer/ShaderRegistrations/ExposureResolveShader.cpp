#include "PCH.h"

#include "Passes/PostProcessing/ExposureShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ExposureCS, "/Engine/Passes/PostProcessing/Exposure.hlsl",
    "main",
    Compute);
