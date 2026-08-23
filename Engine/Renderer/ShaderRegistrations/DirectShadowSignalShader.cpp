#include "PCH.h"

#include "Passes/Lighting/Shadows/DirectShadowSignalShader.h"

IMPLEMENT_GLOBAL_SHADER(
    DirectShadowSignalCS, "/Engine/Passes/Lighting/Shadows/DirectShadowSignal.hlsl",
    "main",
    Compute);
