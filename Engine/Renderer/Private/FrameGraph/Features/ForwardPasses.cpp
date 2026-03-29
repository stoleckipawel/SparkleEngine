#include "../../PCH.h"

#include "FrameGraph/Features/ForwardPasses.h"

#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/Passes/ForwardOpaquePass.h"

namespace FrameGraphFeatures
{
	void AddForwardOpaquePass(
	    FrameGraph& frameGraph,
	    const FrameGraphSceneTargets& sceneTargets,
	    const FrameGraphShadowOutputs& shadowOutputs)
	{
		auto& parameters = frameGraph.AllocPassParameters<ForwardOpaquePass>();
		parameters->BackBuffer = frameGraph.CreateRenderTarget(sceneTargets.BackBuffer);
		parameters->MainDepth = frameGraph.CreateDepthTarget(sceneTargets.MainDepth);
		parameters->ShadowMap0 = frameGraph.Read(shadowOutputs.ShadowMaps[0]);
		parameters->ShadowMap1 = frameGraph.Read(shadowOutputs.ShadowMaps[1]);
		parameters->ShadowMap2 = frameGraph.Read(shadowOutputs.ShadowMaps[2]);
		parameters->ShadowMap3 = frameGraph.Read(shadowOutputs.ShadowMaps[3]);
		parameters->SamplerTable = SamplerReference{.Name = "SceneSamplers"};

		frameGraph.AddRasterPass<ForwardOpaquePass>(ForwardOpaquePass::PassName, parameters);
	}
}  // namespace FrameGraphFeatures
