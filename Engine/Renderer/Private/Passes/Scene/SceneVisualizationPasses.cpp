#include "../../PCH.h"
#include "Passes/Scene/SceneVisualizationPasses.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Debug/VisualizeBuffers.h"

void AddSceneVisualizationPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RenderFrameGraphResources& resources)
{
	if (viewMode >= RenderViewMode::GBufferDiffuse)
	{
		AddVisualizeBuffersPass(builder, settings.RenderExtent, resources);
	}
}
