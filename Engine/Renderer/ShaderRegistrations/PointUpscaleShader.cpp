#include "PCH.h"

#include "Passes/Presentation/Upscaling/PointUpscaleShader.h"

IMPLEMENT_GLOBAL_SHADER(PointUpscaleCS, "/Engine/Passes/Presentation/Upscaling/PointUpscale.hlsl", "main", Compute);
