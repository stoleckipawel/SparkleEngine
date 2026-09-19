#include "PCH.h"

#include "Passes/PostProcessing/Exposure/ExposureReduceSceneShader.h"

IMPLEMENT_GLOBAL_SHADER(ExposureReduceSceneCS, "/Engine/Passes/PostProcessing/Exposure/ExposureReduceScene.hlsl", "main", Compute);
