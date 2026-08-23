#include "PCH.h"

#include "Passes/Debug/VisualizeBuffersShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    VisualizeBuffersCS,
    RendererShaderPackages::VisualizeBuffers,
    "/Engine/Passes/Debug/VisualizeBuffers.hlsl",
    "main",
    Compute);
