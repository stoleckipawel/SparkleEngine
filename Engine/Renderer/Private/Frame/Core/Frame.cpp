#include "../../PCH.h"
#include "Frame/Core/Frame.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "Frame/Deferred/GBuffer.h"
#include "Frame/Lighting/Lighting.h"
#include "Frame/Lighting/LightingRenderTargets.h"
#include "Frame/Presentation/Presentation.h"
#include "Frame/RayTracing/RayTracingScene.h"
#include "Frame/Presentation/Upscaling.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

FrameBuildResult BuildFrame(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    PixelFormat backBufferFormat,
    bool presentToBackBuffer)
{
	FrameAssemblyResourceLayout resources = {};

	const FrameGraphTextureDesc sceneColorDesc =
	    FrameGraphTextureDesc::CreateColor("SceneColor", sceneExtent.Width, sceneExtent.Height, FrameRenderFormats::SceneColor);
	const FrameGraphTextureHandle sceneColor = builder.CreateTexture(sceneColorDesc);

	const FrameGraphTextureDesc finalSceneColorDesc =
	    FrameGraphTextureDesc::CreateColor("FinalSceneColor", sceneExtent.Width, sceneExtent.Height, FrameRenderFormats::SceneColor);
	const FrameGraphTextureHandle finalSceneColor = builder.CreateTexture(finalSceneColorDesc);

	const FrameGraphTextureDesc backBufferDesc =
	    FrameGraphTextureDesc::CreateColor("BackBuffer", sceneExtent.Width, sceneExtent.Height, backBufferFormat);
	const FrameGraphTextureHandle backBuffer = builder.ImportTexture(backBufferDesc, ResourceState::Present);
	resources.Imported.BackBuffer = backBuffer;

	const FrameGraphTextureDesc mainDepthDesc =
	    FrameGraphTextureDesc::CreateDepthStencil("MainDepth", sceneExtent.Width, sceneExtent.Height, FrameRenderFormats::DepthStencil);
	const FrameGraphTextureHandle mainDepth = builder.CreateTexture(mainDepthDesc);

	resources.Transient.Scene = SceneRenderTargets{
	    .SceneColor = sceneColor,
	    .FinalSceneColor = finalSceneColor,
	    .BackBuffer = backBuffer,
	    .MainDepth = mainDepth};
	resources.ViewportProducts.SceneColor = sceneColor;
	resources.ViewportProducts.FinalSceneColor = finalSceneColor;
	resources.ViewportProducts.SceneDepth = mainDepth;

	resources.Transient.GBuffer = CreateGBufferRenderTargets(builder, sceneExtent, resources.Transient.Scene);
	resources.ViewportProducts.Normals = resources.Transient.GBuffer.Normal;
	resources.ViewportProducts.MotionVectors = resources.Transient.GBuffer.MotionVector;
	AddGBufferPass(builder, resources.Transient.GBuffer);

	resources.Persistent.RayTracing = CreateRayTracingSceneFrameGraphResources(builder);
	resources.Persistent.SceneTlas = resources.Persistent.RayTracing.SceneTlas;
	AddRayTracingSceneBuildPasses(builder, resources.Persistent.RayTracing);

	resources.Transient.Lighting = CreateLightingRenderTargets(builder, sceneExtent);
	
	AddLightingPasses(
	    builder,
	    resources.Transient.Scene,
	    resources.Transient.Lighting,
	    resources.Transient.GBuffer,
	    resources.Persistent.RayTracing.SceneTlas);

	AddExternalProviderEvaluationPass(builder, sceneExtent, resources.Transient.Scene, resources.Transient.GBuffer);

	resources.ProviderInputs = FrameAssemblyProviderResources{
	    .HudlessSceneColor = resources.Transient.Scene.SceneColor,
	    .Depth = resources.Transient.Scene.MainDepth,
	    .MotionVectors = resources.Transient.GBuffer.MotionVector,
	    .FinalOutputColor = resources.Transient.Scene.FinalSceneColor,
	    .Exposure = FrameGraphTextureHandle::Invalid(),
	    .Normals = resources.Transient.GBuffer.Normal};

	if (presentToBackBuffer)
	{
		AddPresentationPass(builder, resources.Transient.Scene);
	}

	return FrameBuildResult{.Resources = resources};
}
