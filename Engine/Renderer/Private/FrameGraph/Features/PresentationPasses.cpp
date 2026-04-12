#include "../../PCH.h"

#include "FrameGraph/Features/PresentationPasses.h"

#include "Config/RenderConfig.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/FrameGraph/ResourceState.h"
#include "Renderer/Public/Passes/PassUtilities.h"

#include "Window/Window.h"

namespace FrameGraphFeatures
{
	FrameGraphSceneTargets CreateSceneTargets(FrameGraph& frameGraph, const Window& window)
	{
		const std::uint32_t width = static_cast<std::uint32_t>(window.GetWidth());
		const std::uint32_t height = static_cast<std::uint32_t>(window.GetHeight());

		const FrameGraphTextureDesc backBufferDesc =
		    FrameGraphTextureDesc::CreateColor("BackBuffer", width, height, RenderConfig::BackBufferFormat);
		const TextureHandle backBuffer = frameGraph.ImportTexture(backBufferDesc, ResourceState::Present);

		const FrameGraphTextureDesc mainDepthDesc = FrameGraphTextureDesc::CreateDepthStencil("MainDepth", width, height);
		const TextureHandle mainDepth = frameGraph.CreateTexture(mainDepthDesc);

		return FrameGraphSceneTargets{.BackBuffer = backBuffer, .MainDepth = mainDepth};
	}

	void AddCopyToBackBufferPass(
	    FrameGraph& frameGraph,
	    const FrameGraphPresentationInputs& presentation,
	    const FrameGraphComputeShowcaseOutputs& computeOutputs)
	{
		PassUtilities::AddCopyTexturePass(frameGraph, "CopyComputeClearToBackBuffer", presentation.BackBuffer, computeOutputs.Color);
	}
}  // namespace FrameGraphFeatures
