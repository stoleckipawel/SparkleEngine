#include "PCH.h"

#include "Passes/PostProcessing/Exposure/ExposureShader.h"

IMPLEMENT_GLOBAL_SHADER(ExposureCS, "/Engine/Passes/PostProcessing/Exposure/Exposure.hlsl", "main", Compute);
