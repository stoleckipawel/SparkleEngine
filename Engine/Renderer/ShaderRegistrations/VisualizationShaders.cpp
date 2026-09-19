#include "PCH.h"

#include "Passes/Visualization/GBufferVisualizationShader.h"
#include "Passes/Visualization/GpuSceneVisualizationShader.h"
#include "Passes/Visualization/LightingVisualizationShader.h"

IMPLEMENT_GLOBAL_SHADER(GBufferVisualizationCS, "/Engine/Passes/Visualization/GBufferVisualization.hlsl", "main", Compute);
IMPLEMENT_GLOBAL_SHADER(GpuSceneVisualizationCS, "/Engine/Passes/Visualization/GpuSceneVisualization.hlsl", "main", Compute);
IMPLEMENT_GLOBAL_SHADER(LightingVisualizationCS, "/Engine/Passes/Visualization/LightingVisualization.hlsl", "main", Compute);
