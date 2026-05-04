#include "../PCH.h"
#include "Frame/VisualizeBuffers.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/VisualizeBuffersPass.h"

void BuildVisualizeBuffers(
	FrameGraph& frameGraph,
	const SceneTargets& sceneTargets,
	const LightingTargets& lighting,
	const GBufferTargets& gbuffer)
{
	auto& parameters = frameGraph.AllocPassParameters<VisualizeBuffersPass>();
	VisualizeBuffersPass::DeclareResources(frameGraph, sceneTargets, lighting, gbuffer, parameters);
	frameGraph.AddComputePass<VisualizeBuffersPass>(VisualizeBuffersPass::PassName, parameters);
}