#include "../PCH.h"
#include "Frame/Frame.h"

#include "Config/RenderConfig.h"
#include "Frame/GBuffer.h"
#include "Frame/Lighting.h"
#include "Frame/LightingTargets.h"
#include "Frame/Presentation.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

FrameBuildResult BuildFrame(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, bool presentToBackBuffer)
{
	const FrameGraphTextureDesc sceneColorDesc =
	    FrameGraphTextureDesc::CreateColor("SceneColor", sceneExtent.Width, sceneExtent.Height, RenderConfig::SceneColorFormat);
	const FrameGraphTextureHandle sceneColor = builder.CreateTexture(sceneColorDesc);

	const FrameGraphTextureDesc backBufferDesc =
	    FrameGraphTextureDesc::CreateColor("BackBuffer", sceneExtent.Width, sceneExtent.Height, RenderConfig::BackBufferFormat);
	const FrameGraphTextureHandle backBuffer = builder.ImportTexture(backBufferDesc, ResourceState::Present);

	const FrameGraphTextureDesc mainDepthDesc =
	    FrameGraphTextureDesc::CreateDepthStencil("MainDepth", sceneExtent.Width, sceneExtent.Height);
	const FrameGraphTextureHandle mainDepth = builder.CreateTexture(mainDepthDesc);

	const SceneTargets sceneTargets{.SceneColor = sceneColor, .BackBuffer = backBuffer, .MainDepth = mainDepth};

	const GBufferTargets gbuffer = CreateGBufferTargets(builder, sceneExtent, sceneTargets);
	AddGBufferPass(builder, gbuffer);

	const LightingTargets lighting = CreateLightingTargets(builder, sceneExtent);
	AddLightingPasses(builder, sceneTargets, lighting, gbuffer);

	if (presentToBackBuffer)
	{
		AddPresentationPass(builder, sceneTargets);
	}

	return FrameBuildResult{.Targets = sceneTargets, .GBuffer = gbuffer};
}
