#include "PCH.h"

#include "Passes/Lighting/Shadows/DirectShadowSignalShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectShadowSignalCS,
    RendererShaderPackages::DirectShadowSignal,
    "/Engine/Passes/Lighting/Shadows/DirectShadowSignal.hlsl",
    "main",
    Compute);
