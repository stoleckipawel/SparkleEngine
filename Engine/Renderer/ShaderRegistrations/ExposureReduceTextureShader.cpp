#include "PCH.h"

#include "Passes/PostProcessing/Exposure/ExposureReduceTextureShader.h"

IMPLEMENT_GLOBAL_SHADER(ExposureReduceTextureCS, "/Engine/Passes/PostProcessing/Exposure/ExposureReduceTexture.hlsl", "main", Compute);
