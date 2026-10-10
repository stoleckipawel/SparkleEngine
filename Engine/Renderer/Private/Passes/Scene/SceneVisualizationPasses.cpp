#include "../../PCH.h"
#include "Passes/Scene/SceneVisualizationPasses.h"

#include "Frame/RenderFrame.h"
#include "Passes/Visualization/GBufferVisualization.h"
#include "Passes/Visualization/GpuSceneVisualization.h"
#include "Passes/Visualization/LightingVisualization.h"

void AddSceneVisualizationPasses(FrameGraphBuilder& builder, const RenderFrame& frame, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	switch (frame.View.viewMode)
	{
		case RenderViewMode::GBufferDiffuse:
		case RenderViewMode::GBufferWorldNormal:
		case RenderViewMode::GBufferWorldTangent:
		case RenderViewMode::GBufferRoughness:
		case RenderViewMode::GBufferMetallic:
		case RenderViewMode::GBufferEmissive:
		case RenderViewMode::GBufferAmbientOcclusion:
		case RenderViewMode::GBufferSubsurfaceColor:
		case RenderViewMode::GBufferSubsurfaceStrength:
			AddGBufferVisualizationPass(builder, frame, sceneExtent, resources);
			return;
		case RenderViewMode::DirectDiffuse:
		case RenderViewMode::DirectSpecular:
		case RenderViewMode::DirectSubsurface:
		case RenderViewMode::IndirectDiffuse:
		case RenderViewMode::IndirectSpecular:
			AddLightingVisualizationPass(builder, frame, sceneExtent, resources);
			return;
		case RenderViewMode::GpuSceneInstances:
			AddGpuSceneVisualizationPass(builder, sceneExtent, resources);
			return;
		default:
			return;
	}
}
