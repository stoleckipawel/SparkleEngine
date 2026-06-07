#include "../PCH.h"
#include "Frame/Frame.h"

#include "Config/RenderConfig.h"
#include "Frame/GBuffer.h"
#include "Frame/Lighting.h"
#include "Frame/LightingRenderTargets.h"
#include "Frame/Presentation.h"
#include "Frame/RayTracingScene.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

FrameBuildResult BuildFrame(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, bool presentToBackBuffer)
{
	const FrameGraphTextureDesc sceneColorDesc = FrameGraphTextureDesc::CreateColor("SceneColor", sceneExtent.Width, sceneExtent.Height, RenderConfig::SceneColorFormat);
	const FrameGraphTextureHandle sceneColor = builder.CreateTexture(sceneColorDesc);

	const FrameGraphTextureDesc backBufferDesc = FrameGraphTextureDesc::CreateColor("BackBuffer", sceneExtent.Width, sceneExtent.Height, RenderConfig::BackBufferFormat);
	const FrameGraphTextureHandle backBuffer = builder.ImportTexture(backBufferDesc, ResourceState::Present);

	const FrameGraphTextureDesc mainDepthDesc = FrameGraphTextureDesc::CreateDepthStencil("MainDepth", sceneExtent.Width, sceneExtent.Height);
	const FrameGraphTextureHandle mainDepth = builder.CreateTexture(mainDepthDesc);

	const SceneRenderTargets sceneTargets{.SceneColor = sceneColor, .BackBuffer = backBuffer, .MainDepth = mainDepth};

	const GBufferRenderTargets gbuffer = CreateGBufferRenderTargets(builder, sceneExtent, sceneTargets);
	AddGBufferPass(builder, gbuffer);

	const FrameGraphAccelerationStructureHandle sceneTlas =
	    builder.ReservePersistentAccelerationStructure(FrameGraphAccelerationStructureDesc::Create("SceneTlas"));
	AddRayTracingSceneBuildPass(builder, sceneTlas);

	const LightingRenderTargets lighting = CreateLightingRenderTargets(builder, sceneExtent);
	AddLightingPasses(builder, sceneTargets, lighting, gbuffer, sceneTlas);

	if (presentToBackBuffer)
	{
		AddPresentationPass(builder, sceneTargets);
	}

	return FrameBuildResult{.Scene = sceneTargets, .GBuffer = gbuffer, .SceneTlas = sceneTlas};
}
