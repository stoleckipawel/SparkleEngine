#include "../../PCH.h"
#include "Frame/Core/FrameSceneResources.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

void CreateFrameSceneResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    PixelFormat backBufferFormat,
    FrameAssemblyResourceLayout& resources)
{
	const FrameGraphTextureHandle sceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "SceneColor",
	        renderExtent.Width,
	        renderExtent.Height,
	        FrameRenderFormats::SceneColor));

	const FrameGraphTextureHandle sceneDepth = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "SceneDepth",
	        renderExtent.Width,
	        renderExtent.Height,
	        FrameRenderFormats::SceneDepth));

	const FrameGraphTextureHandle reconstructedSceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ReconstructedSceneColor",
	        renderExtent.Width,
	        renderExtent.Height,
	        FrameRenderFormats::SceneColor));

	const FrameGraphTextureHandle finalSceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "FinalSceneColor",
	        outputExtent.Width,
	        outputExtent.Height,
	        FrameRenderFormats::SceneColor));

	const FrameGraphTextureHandle backBuffer = builder.ImportTexture(
	    FrameGraphTextureDesc::CreateColor("BackBuffer", outputExtent.Width, outputExtent.Height, backBufferFormat),
	    ResourceState::Present);

	const FrameGraphTextureHandle exposure = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("Exposure", 1, 1, PixelFormat::R32G32B32A32_Float));
	const FrameGraphTextureHandle previousExposure = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("PreviousExposureHistory", 1, 1, PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentExposure = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("CurrentExposureHistory", 1, 1, PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle previousDirectLightReservoirSample = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "PreviousDirectLightReservoirSample",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle previousDirectLightReservoirWeight = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "PreviousDirectLightReservoirWeight",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle previousDirectLightReservoirSurface = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "PreviousDirectLightReservoirSurface",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R16G16B16A16_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentDirectLightReservoirSample = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "CurrentDirectLightReservoirSample",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentDirectLightReservoirWeight = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "CurrentDirectLightReservoirWeight",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentDirectLightReservoirSurface = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "CurrentDirectLightReservoirSurface",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R16G16B16A16_Float),
	    ResourceState::ShaderResource);

	resources.Transient.Scene = SceneRenderTargets{
	    .SceneColor = sceneColor,
	    .SceneDepth = sceneDepth,
	    .ReconstructedSceneColor = reconstructedSceneColor,
	    .FinalSceneColor = finalSceneColor,
	    .BackBuffer = backBuffer};
	resources.Transient.Exposure = exposure;
	resources.History.PreviousExposure = previousExposure;
	resources.History.CurrentExposure = currentExposure;
	resources.History.PreviousDirectLightReservoirSample = previousDirectLightReservoirSample;
	resources.History.PreviousDirectLightReservoirWeight = previousDirectLightReservoirWeight;
	resources.History.PreviousDirectLightReservoirSurface = previousDirectLightReservoirSurface;
	resources.History.CurrentDirectLightReservoirSample = currentDirectLightReservoirSample;
	resources.History.CurrentDirectLightReservoirWeight = currentDirectLightReservoirWeight;
	resources.History.CurrentDirectLightReservoirSurface = currentDirectLightReservoirSurface;
	resources.ViewportProducts.SceneColor = sceneColor;
	resources.ViewportProducts.SceneDepth = sceneDepth;
	resources.ViewportProducts.FinalSceneColor = finalSceneColor;
	resources.ViewportProducts.Exposure = exposure;
}
