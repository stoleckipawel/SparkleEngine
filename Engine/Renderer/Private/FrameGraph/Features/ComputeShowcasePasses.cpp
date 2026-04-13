#include "../../PCH.h"

#include "FrameGraph/Features/ComputeShowcasePasses.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/ComputeClearPass.h"

namespace FrameGraphFeatures
{
	FrameGraphComputeShowcaseOutputs AddComputeClearShowcasePass(
	    FrameGraph& frameGraph,
	    const std::uint32_t width,
	    const std::uint32_t height)
	{
		FrameGraphComputeShowcaseOutputs outputs{};
		outputs.Color = frameGraph.CreateTexture(
		    FrameGraphTextureDesc::CreateColor("ComputeClearExample", width, height, RenderConfig::BackBufferFormat));

		auto& parameters = frameGraph.AllocPassParameters<ComputeClearPass>();
		parameters->Output = frameGraph.CreateUAV(outputs.Color);

		frameGraph.AddComputePass<ComputeClearPass>("ComputeClearExample", parameters, width, height);

		return outputs;
	}
}  // namespace FrameGraphFeatures
