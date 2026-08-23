#include "PCH.h"

#include "Passes/Presentation/ToneMappingShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ToneMappingCS,
    RendererShaderPackages::ToneMapping,
    "/Engine/Passes/Presentation/ToneMapping.hlsl",
    "main",
    Compute);
