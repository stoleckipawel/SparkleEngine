#include "PCH.h"

#include "Passes/Presentation/Upscaling/LinearUpscaleShader.h"

IMPLEMENT_GLOBAL_SHADER(LinearUpscaleCS, "/Engine/Passes/Presentation/Upscaling/LinearUpscale.hlsl", "main", Compute);
