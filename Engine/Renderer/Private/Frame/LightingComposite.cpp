#include "../PCH.h"
#include "Frame/LightingComposite.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/LightingCompositePass.h"

void AddLightingCompositePass(
    FrameGraph& frameGraph,
    const SceneTargets& sceneTargets,
    const LightingTargets& lighting,
    const GBufferTargets& gbuffer)
{
	auto& parameters = frameGraph.AllocPassParameters<LightingCompositePass>();
	LightingCompositePass::DeclareResources(frameGraph, sceneTargets, lighting, gbuffer, parameters);
	frameGraph.AddComputePass<LightingCompositePass>(LightingCompositePass::PassName, parameters);
}