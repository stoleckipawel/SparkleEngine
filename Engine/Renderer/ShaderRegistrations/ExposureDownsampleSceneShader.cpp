#include "PCH.h"

#include "Passes/PostProcessing/ExposureDownsampleSceneShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ExposureDownsampleSceneCS, "/Engine/Passes/PostProcessing/ExposureDownsampleScene.hlsl",
    "main",
    Compute);
