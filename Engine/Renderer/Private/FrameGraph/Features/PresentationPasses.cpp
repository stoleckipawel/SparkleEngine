#include "../../PCH.h"

#include "FrameGraph/Features/PresentationPasses.h"

#include "Config/RenderConfig.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/FrameGraph/ResourceState.h"
#include "Passes/PassUtilities.h"

#include "Window/Window.h"

namespace FrameGraphFeatures
{
	FrameGraphSceneTargets CreateSceneTargets(FrameGraph& frameGraph, const Window& window, const RenderViewportExtent& sceneExtent)
	{
		const std::uint32_t width = static_cast<std::uint32_t>(window.GetWidth());
		const std::uint32_t height = static_cast<std::uint32_t>(window.GetHeight());
		const std::uint32_t sceneWidth = sceneExtent.IsValid() ? sceneExtent.Width : width;
		const std::uint32_t sceneHeight = sceneExtent.IsValid() ? sceneExtent.Height : height;

		const FrameGraphTextureDesc sceneColorDesc =
		    FrameGraphTextureDesc::CreateColor("SceneColor", sceneWidth, sceneHeight, RenderConfig::BackBufferFormat);
		const TextureHandle sceneColor = frameGraph.CreateTexture(sceneColorDesc);

		const FrameGraphTextureDesc backBufferDesc =
		    FrameGraphTextureDesc::CreateColor("BackBuffer", width, height, RenderConfig::BackBufferFormat);
		const TextureHandle backBuffer = frameGraph.ImportTexture(backBufferDesc, ResourceState::Present);

		const FrameGraphTextureDesc mainDepthDesc = FrameGraphTextureDesc::CreateDepthStencil("MainDepth", sceneWidth, sceneHeight);
		const TextureHandle mainDepth = frameGraph.CreateTexture(mainDepthDesc);

		return FrameGraphSceneTargets{.SceneColor = sceneColor, .BackBuffer = backBuffer, .MainDepth = mainDepth};
	}

	void AddCopyToBackBufferPass(FrameGraph& frameGraph, const FrameGraphSceneTargets& sceneTargets)
	{
		PassUtilities::AddCopyTexturePass(frameGraph, "CopySceneColorToBackBuffer", sceneTargets.BackBuffer, sceneTargets.SceneColor);
	}
}  // namespace FrameGraphFeatures
