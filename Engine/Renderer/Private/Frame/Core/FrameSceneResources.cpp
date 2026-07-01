#include "../../PCH.h"
#include "Frame/Core/FrameSceneResources.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

void CreateFrameSceneResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    PixelFormat backBufferFormat,
    FrameAssemblyResourceLayout& resources)
{
	const FrameGraphTextureHandle sceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "SceneColor",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        FrameRenderFormats::SceneColor));

	const FrameGraphTextureHandle finalSceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "FinalSceneColor",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        FrameRenderFormats::SceneColor));

	const FrameGraphTextureHandle backBuffer = builder.ImportTexture(
	    FrameGraphTextureDesc::CreateColor("BackBuffer", sceneExtent.Width, sceneExtent.Height, backBufferFormat),
	    ResourceState::Present);

	const FrameGraphTextureHandle mainDepth = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateDepthStencil(
	        "MainDepth",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        FrameRenderFormats::DepthStencil));

	const FrameGraphTextureHandle exposure = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("Exposure", 1, 1, PixelFormat::R32G32B32A32_Float));
	const FrameGraphTextureHandle previousExposure = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("PreviousExposureHistory", 1, 1, PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentExposure = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("CurrentExposureHistory", 1, 1, PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);

	resources.Imported.BackBuffer = backBuffer;
	resources.Transient.Scene = SceneRenderTargets{
	    .SceneColor = sceneColor,
	    .FinalSceneColor = finalSceneColor,
	    .BackBuffer = backBuffer,
	    .MainDepth = mainDepth};
	resources.Transient.Exposure = exposure;
	resources.History.PreviousExposure = previousExposure;
	resources.History.CurrentExposure = currentExposure;
	resources.ViewportProducts.SceneColor = sceneColor;
	resources.ViewportProducts.FinalSceneColor = finalSceneColor;
	resources.ViewportProducts.Exposure = exposure;
}
