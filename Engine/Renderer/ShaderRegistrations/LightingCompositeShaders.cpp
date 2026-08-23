#include "PCH.h"

#include "Passes/Lighting/LightingCompositeShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    LightingCompositeCS,
    RendererShaderPackages::LightingComposite,
    "/Engine/Passes/Lighting/LightingComposite.hlsl",
    "main",
    Compute);
