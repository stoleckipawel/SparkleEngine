#include "PCH.h"

#include "Passes/Utility/ComputeClear.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ComputeClearCS,
    RendererShaderPackages::ComputeClear,
    "/Engine/Passes/Compute/ComputeClear.hlsl",
    "main",
    Compute);
