#include "../../PCH.h"

#include "FrameGraph/Features/ForwardPasses.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/ForwardOpaquePass.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"

namespace FrameGraphFeatures
{
	void AddForwardOpaquePass(
	    FrameGraph& frameGraph,
	    const FrameGraphSceneTargets& sceneTargets,
	    const FrameGraphShadowOutputs& shadowOutputs)
	{
		auto& parameters = frameGraph.AllocPassParameters<ForwardOpaquePass>();
		parameters->BackBuffer = frameGraph.CreateRenderTarget(sceneTargets.SceneColor);
		parameters->MainDepth = frameGraph.CreateDepthTarget(sceneTargets.MainDepth);
		parameters->ShadowMap0 = frameGraph.CreateSRV(shadowOutputs.ShadowMaps[0]);
		parameters->ShadowMap1 = frameGraph.CreateSRV(shadowOutputs.ShadowMaps[1]);
		parameters->ShadowMap2 = frameGraph.CreateSRV(shadowOutputs.ShadowMaps[2]);
		parameters->ShadowMap3 = frameGraph.CreateSRV(shadowOutputs.ShadowMaps[3]);
		parameters->SamplerAniso16xWrap = RhiSamplerDesc{
		    .MaxAnisotropy = RhiSamplerAnisotropy::X16};
		parameters->SamplerLinearNoMipClamp = RhiSamplerDesc{
		    .MipFilter = RhiSamplerMipFilter::None,
		    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};

		frameGraph.AddRasterPass<ForwardOpaquePass>(ForwardOpaquePass::PassName, parameters);
	}
}  // namespace FrameGraphFeatures
