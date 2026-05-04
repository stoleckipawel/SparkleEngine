#include "../PCH.h"
#include "Frame/Frame.h"

#include "Config/RenderConfig.h"
#include "Frame/GBuffer.h"
#include "Frame/Lighting.h"
#include "Frame/Presentation.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

FrameBuildResult BuildFrame(
	FrameGraph& frameGraph,
	RenderViewportExtent sceneExtent,
	bool presentToBackBuffer)
{
	const FrameGraphTextureDesc sceneColorDesc =
	    FrameGraphTextureDesc::CreateColor("SceneColor", sceneExtent.Width, sceneExtent.Height, RenderConfig::SceneColorFormat);
	const TextureHandle sceneColor = frameGraph.CreateTexture(sceneColorDesc);

	const FrameGraphTextureDesc backBufferDesc =
	    FrameGraphTextureDesc::CreateColor("BackBuffer", sceneExtent.Width, sceneExtent.Height, RenderConfig::BackBufferFormat);
	const TextureHandle backBuffer = frameGraph.ImportTexture(backBufferDesc, ResourceState::Present);

	const FrameGraphTextureDesc mainDepthDesc = FrameGraphTextureDesc::CreateDepthStencil("MainDepth", sceneExtent.Width, sceneExtent.Height);
	const TextureHandle mainDepth = frameGraph.CreateTexture(mainDepthDesc);

	const SceneTargets sceneTargets{.SceneColor = sceneColor, .BackBuffer = backBuffer, .MainDepth = mainDepth};

	const GBufferTargets gbuffer = BuildGBuffer(frameGraph, sceneExtent, sceneTargets);

	BuildLighting(frameGraph, sceneExtent, sceneTargets, gbuffer);

	if (presentToBackBuffer)
	{
		BuildPresentation(frameGraph, sceneTargets);
	}

	return FrameBuildResult{.Targets = sceneTargets, .GBuffer = gbuffer};
}
