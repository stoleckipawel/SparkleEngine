#include "PCH.h"

#include "Passes/Presentation/LinearUpscaleShader.h"

IMPLEMENT_GLOBAL_SHADER(
    LinearUpscaleCS, "/Engine/Passes/Presentation/LinearUpscale.hlsl",
    "main",
    Compute);
