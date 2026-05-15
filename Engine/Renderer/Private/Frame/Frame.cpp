#include "../PCH.h"
#include "Frame/Frame.h"

#include "Config/RenderConfig.h"
#include "Frame/GBuffer.h"
#include "Frame/Lighting.h"
#include "Frame/LightingTargets.h"
#include "Frame/Presentation.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

FrameBuildResult BuildFrame(FrameGraph& frameGraph, RenderViewportExtent sceneExtent, bool presentToBackBuffer)
{
	const FrameGraphTextureDesc sceneColorDesc =
	    FrameGraphTextureDesc::CreateColor("SceneColor", sceneExtent.Width, sceneExtent.Height, RenderConfig::SceneColorFormat);
	const FrameGraphTextureHandle sceneColor = frameGraph.CreateTexture(sceneColorDesc);

	const FrameGraphTextureDesc backBufferDesc =
	    FrameGraphTextureDesc::CreateColor("BackBuffer", sceneExtent.Width, sceneExtent.Height, RenderConfig::BackBufferFormat);
	const FrameGraphTextureHandle backBuffer = frameGraph.ImportTexture(backBufferDesc, ResourceState::Present);

	const FrameGraphTextureDesc mainDepthDesc =
	    FrameGraphTextureDesc::CreateDepthStencil("MainDepth", sceneExtent.Width, sceneExtent.Height);
	const FrameGraphTextureHandle mainDepth = frameGraph.CreateTexture(mainDepthDesc);

	const SceneTargets sceneTargets{.SceneColor = sceneColor, .BackBuffer = backBuffer, .MainDepth = mainDepth};

	const GBufferTargets gbuffer = CreateGBufferTargets(frameGraph, sceneExtent, sceneTargets);
	AddGBufferPass(frameGraph, gbuffer);

	const LightingTargets lighting = CreateLightingTargets(frameGraph, sceneExtent);
	AddLightingPasses(frameGraph, sceneTargets, lighting, gbuffer);

	if (presentToBackBuffer)
	{
		AddPresentationPass(frameGraph, sceneTargets);
	}

	return FrameBuildResult{.Targets = sceneTargets, .GBuffer = gbuffer};
}
