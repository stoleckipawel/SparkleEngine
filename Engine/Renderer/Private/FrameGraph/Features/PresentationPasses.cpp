#include "../../PCH.h"

#include "FrameGraph/Features/PresentationPasses.h"

#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "Renderer/Public/Passes/PassUtilities.h"

#include "D3D12DescriptorHeapManager.h"

#include "UI.h"
#include "Window.h"

namespace FrameGraphFeatures
{
FrameGraphSceneTargets CreateSceneTargets(FrameGraph& frameGraph, const Window& window)
{
	const std::uint32_t width = static_cast<std::uint32_t>(window.GetWidth());
	const std::uint32_t height = static_cast<std::uint32_t>(window.GetHeight());

	const FrameGraphTextureDesc backBufferDesc = FrameGraphTextureDesc::CreateColor(
	    "BackBuffer",
	    width,
	    height,
	    RenderConfig::BackBufferFormat);
	const TextureHandle backBuffer = frameGraph.ImportTexture(backBufferDesc, ResourceState::Present);

	const FrameGraphTextureDesc mainDepthDesc = FrameGraphTextureDesc::CreateDepthStencil(
	    "MainDepth",
	    width,
	    height);
	const TextureHandle mainDepth = frameGraph.CreateTexture(mainDepthDesc);

	return FrameGraphSceneTargets{.BackBuffer = backBuffer, .MainDepth = mainDepth};
}

void AddCopyToBackBufferPass(
    FrameGraph& frameGraph,
    const FrameGraphPresentationInputs& presentation,
    const FrameGraphComputeShowcaseOutputs& computeOutputs)
{
	PassUtilities::AddCopyTexturePass(
	    frameGraph,
	    "CopyComputeClearToBackBuffer",
	    presentation.BackBuffer,
	    computeOutputs.Color);
}

void AddUiCompositionPass(
    FrameGraph& frameGraph,
    UI& ui,
    const FrameGraphPresentationInputs& presentation)
{
	frameGraph.AddPass(
	    "UIComposition",
	    FrameGraphPassFlags::Raster,
	    [backBuffer = presentation.BackBuffer](PassBuilder& builder)
	    {
		    builder.Write(backBuffer, ResourceUsage::RenderTarget);
	    },
	    [&ui, backBuffer = presentation.BackBuffer](RenderGraphPassContext& context)
	    {
		    context.Runtime.DescriptorHeapManager.SetShaderVisibleHeaps(context.Commands);
		    context.Graph.BindRenderTarget(context.Commands, backBuffer);
		    ui.Render(context.Commands.GetCommandList());
	    });
}
}
