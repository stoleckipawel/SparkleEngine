#include "PCH.h"

#include "Passes/Presentation/LinearUpscaleShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    LinearUpscaleCS,
    RendererShaderPackages::LinearUpscale,
    "/Engine/Passes/Presentation/LinearUpscale.hlsl",
    "main",
    Compute);
