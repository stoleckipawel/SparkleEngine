#include "../../PCH.h"
#include "Passes/Scene/SceneVisualizationPasses.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Visualization/GBufferVisualization.h"
#include "Passes/Visualization/GpuSceneVisualization.h"
#include "Passes/Visualization/LightingVisualization.h"

void AddSceneVisualizationPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RenderFrameGraphResources& resources)
{
	AddGBufferVisualizationPass(builder, settings.RenderExtent, viewMode, resources);
	AddLightingVisualizationPass(builder, settings.RenderExtent, viewMode, resources);
	AddGpuSceneVisualizationPass(builder, settings.RenderExtent, viewMode, resources);
}
