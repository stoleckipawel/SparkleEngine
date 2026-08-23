#include "PCH.h"

#include "Passes/GBuffer/SceneDepthShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    SceneDepthCS,
    RendererShaderPackages::SceneDepth,
    "/Engine/Passes/GBuffer/SceneDepth.hlsl",
    "main",
    Compute);
