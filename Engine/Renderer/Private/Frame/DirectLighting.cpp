#include "../PCH.h"
#include "Frame/DirectLighting.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/DirectLightingPass.h"

void BuildDirectLighting(
    FrameGraph& frameGraph,
	const LightingTargets& lighting,
    const GBufferTargets& gbuffer)
{
	auto& parameters = frameGraph.AllocPassParameters<DirectLightingPass>();
	DirectLightingPass::DeclareResources(frameGraph, lighting, gbuffer, parameters);

	frameGraph.AddComputePass<DirectLightingPass>(DirectLightingPass::PassName, parameters);
}
