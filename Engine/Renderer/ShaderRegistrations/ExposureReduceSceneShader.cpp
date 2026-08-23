#include "PCH.h"

#include "Passes/PostProcessing/ExposureReduceSceneShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureReduceSceneCS,
    RendererShaderPackages::ExposureReduceScene,
    "/Engine/Passes/PostProcessing/ExposureReduceScene.hlsl",
    "main",
    Compute);
