#include "PCH.h"

#include "Passes/Presentation/OutputEncodingShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    OutputEncodingCS,
    RendererShaderPackages::OutputEncoding,
    "/Engine/Passes/Presentation/OutputEncoding.hlsl",
    "main",
    Compute);
