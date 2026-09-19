#include "PCH.h"

#include "Passes/PostProcessing/Exposure/ExposureDownsampleSceneShader.h"

IMPLEMENT_GLOBAL_SHADER(ExposureDownsampleSceneCS, "/Engine/Passes/PostProcessing/Exposure/ExposureDownsampleScene.hlsl", "main", Compute);
