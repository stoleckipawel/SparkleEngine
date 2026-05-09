#include "../PCH.h"
#include "Frame/IndirectLighting.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/IndirectLightingPass.h"

void BuildIndirectLighting(FrameGraph& frameGraph, const LightingTargets& lighting)
{
	auto& parameters = frameGraph.AllocPassParameters<IndirectLightingPass>();
	IndirectLightingPass::DeclareResources(frameGraph, lighting, parameters);
	frameGraph.AddComputePass<IndirectLightingPass>(IndirectLightingPass::PassName, parameters);
}