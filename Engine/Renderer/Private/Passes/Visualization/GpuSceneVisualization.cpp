#include "../../PCH.h"
#include "Passes/Visualization/GpuSceneVisualization.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Visualization/GpuSceneVisualizationShader.h"

void AddGpuSceneVisualizationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewMode viewMode,
    const RenderFrameGraphResources& resources)
{
	if (viewMode != RenderViewMode::GpuSceneInstances)
	{
		return;
	}

	auto& parameters = builder.AllocParameters<GpuSceneVisualizationCS>();
	parameters->SceneColor = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);

	builder.Dispatch<GpuSceneVisualizationCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
