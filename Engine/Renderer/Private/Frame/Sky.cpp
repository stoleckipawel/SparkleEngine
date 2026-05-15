#include "../PCH.h"
#include "Frame/Sky.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/SkyPass.h"

void AddSkyPass(FrameGraph& frameGraph, const SceneTargets& sceneTargets, const GBufferTargets& gbuffer)
{
	auto& parameters = frameGraph.AllocPassParameters<SkyPass>();
	SkyPass::DeclareResources(frameGraph, sceneTargets, gbuffer, parameters);
	frameGraph.AddComputePass<SkyPass>(SkyPass::PassName, parameters);
}