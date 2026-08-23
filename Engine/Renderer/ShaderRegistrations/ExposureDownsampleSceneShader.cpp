#include "PCH.h"

#include "Passes/PostProcessing/ExposureDownsampleSceneShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureDownsampleSceneCS,
    RendererShaderPackages::ExposureDownsampleScene,
    "/Engine/Passes/PostProcessing/ExposureDownsampleScene.hlsl",
    "main",
    Compute);
