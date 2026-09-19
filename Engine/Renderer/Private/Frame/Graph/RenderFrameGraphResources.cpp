#include "../../PCH.h"
#include "Frame/Graph/RenderFrameGraphResources.h"

#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

RenderFrameGraphResources CreateRenderFrameGraphResources(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings)
{
	RenderFrameGraphResources resources;

	resources.Transient.Scene.SceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "SceneColor",
	        settings.RenderExtent.Width,
	        settings.RenderExtent.Height,
	        RenderFrameGraphFormats::SceneColor));

	resources.Presentation.SceneColorInput = resources.Transient.Scene.SceneColor;

	resources.Transient.Scene.SceneDepth = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "SceneDepth",
	        settings.RenderExtent.Width,
	        settings.RenderExtent.Height,
	        RenderFrameGraphFormats::SceneDepth));

	resources.Transient.Exposure =
	    builder.CreateTexture(FrameGraphTextureDesc::CreateColor("Exposure", 1, 1, PixelFormat::R32G32B32A32_Float));

	resources.ImportedScene.Sky = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("Sky", 1, 1, PixelFormat::R8G8B8A8_UNorm),
	    ResourceState::ShaderResource);

	resources.ImportedScene.Scene = DeclareRenderSceneGpuResources(builder);
	resources.History = DeclareFrameHistoryResources(builder);

	if (settings.PresentationTarget == FramePresentationTarget::BackBuffer)
	{
		resources.Presentation.BackBuffer = builder.ImportBackBuffer(
		    FrameGraphTextureDesc::CreateColor(
		        "BackBuffer",
		        settings.OutputExtent.Width,
		        settings.OutputExtent.Height,
		        settings.OutputFormat),
		    ResourceState::Present);
	}

	resources.ViewportProducts.SceneDepth = resources.Transient.Scene.SceneDepth;
	return resources;
}

FrameGraphTextureHandle CreateResolvedSceneColorTarget(FrameGraphBuilder& builder, RenderViewportExtent outputExtent)
{
	return builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ResolvedSceneColor",
	        outputExtent.Width,
	        outputExtent.Height,
	        RenderFrameGraphFormats::SceneColor));
}
