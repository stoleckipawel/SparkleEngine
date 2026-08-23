#include "PCH.h"

#include "Passes/Lighting/LightingCompositeShader.h"

IMPLEMENT_GLOBAL_SHADER(
    LightingCompositeCS, "/Engine/Passes/Lighting/LightingComposite.hlsl",
    "main",
    Compute);
