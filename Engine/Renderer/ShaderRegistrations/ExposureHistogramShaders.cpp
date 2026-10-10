#include "PCH.h"

#include "Passes/PostProcessing/Exposure/ExposureHistogramShaders.h"

IMPLEMENT_GLOBAL_SHADER(ExposureHistogramClearCS, "/Engine/Passes/PostProcessing/Exposure/ExposureHistogramClear.hlsl", "main", Compute);
IMPLEMENT_GLOBAL_SHADER(ExposureHistogramBuildCS, "/Engine/Passes/PostProcessing/Exposure/ExposureHistogramBuild.hlsl", "main", Compute);
IMPLEMENT_GLOBAL_SHADER(ExposureHistogramResolveCS, "/Engine/Passes/PostProcessing/Exposure/ExposureHistogramResolve.hlsl", "main", Compute);
