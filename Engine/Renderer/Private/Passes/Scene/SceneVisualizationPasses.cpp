#include "../../PCH.h"
#include "Passes/Scene/SceneVisualizationPasses.h"

#include "Passes/Visualization/GBufferVisualization.h"
#include "Passes/Visualization/GpuSceneVisualization.h"
#include "Passes/Visualization/LightingVisualization.h"

void AddSceneVisualizationPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewMode viewMode,
    RenderFrameGraphResources& resources)
{
	AddGBufferVisualizationPass(builder, sceneExtent, viewMode, resources);
	AddLightingVisualizationPass(builder, sceneExtent, viewMode, resources);
	AddGpuSceneVisualizationPass(builder, sceneExtent, viewMode, resources);
}
