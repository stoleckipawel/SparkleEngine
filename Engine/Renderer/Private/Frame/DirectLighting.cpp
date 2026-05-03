#include "../PCH.h"
#include "Frame/DirectLighting.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/DeferredLightingPass.h"

void BuildDirectLighting(
    FrameGraph& frameGraph,
    const SceneTargets& sceneTargets,
    const GBufferTargets& gbuffer)
{
	auto& parameters = frameGraph.AllocPassParameters<DeferredLightingPass>();
	DeferredLightingPass::DeclareResources(frameGraph, sceneTargets, gbuffer, parameters);

	frameGraph.AddComputePass<DeferredLightingPass>(DeferredLightingPass::PassName, parameters);
}
