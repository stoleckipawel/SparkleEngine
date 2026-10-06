#include "../../PCH.h"
#include "Passes/Scene/SceneVisualizationPasses.h"

#include "Passes/Visualization/GBufferVisualization.h"
#include "Passes/Visualization/GpuSceneVisualization.h"
#include "Passes/Visualization/LightingVisualization.h"

void AddSceneVisualizationPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	AddGBufferVisualizationPass(builder, sceneExtent, resources);
	AddLightingVisualizationPass(builder, sceneExtent, resources);
	AddGpuSceneVisualizationPass(builder, sceneExtent, resources);
}
