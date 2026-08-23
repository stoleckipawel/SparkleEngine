#include "PCH.h"

#include "Passes/PostProcessing/ExposureReduceSceneShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ExposureReduceSceneCS, "/Engine/Passes/PostProcessing/ExposureReduceScene.hlsl",
    "main",
    Compute);
