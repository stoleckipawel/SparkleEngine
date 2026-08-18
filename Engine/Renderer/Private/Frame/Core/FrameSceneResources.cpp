#include "../../PCH.h"
#include "Frame/Core/FrameSceneResources.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Resources/History/FrameHistory.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "SceneData/RenderSceneGpuData.h"

void CreateFrameSceneResources(FrameGraphBuilder& builder, const FrameBuildSettings& settings, FrameAssemblyResourceLayout& resources)
{
	const FrameGraphTextureHandle sceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "SceneColor",
	        settings.RenderExtent.Width,
	        settings.RenderExtent.Height,
	        FrameRenderFormats::SceneColor));

	const FrameGraphTextureHandle sceneDepth = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "SceneDepth",
	        settings.RenderExtent.Width,
	        settings.RenderExtent.Height,
	        FrameRenderFormats::SceneDepth));

	const FrameGraphTextureHandle finalSceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "FinalSceneColor",
	        settings.OutputExtent.Width,
	        settings.OutputExtent.Height,
	        FrameRenderFormats::SceneColor));

	const FrameGraphTextureHandle backBuffer = builder.ImportBackBuffer(
	    FrameGraphTextureDesc::CreateColor("BackBuffer", settings.OutputExtent.Width, settings.OutputExtent.Height, settings.OutputFormat),
	    ResourceState::Present);

	const FrameGraphTextureHandle exposure =
	    builder.CreateTexture(FrameGraphTextureDesc::CreateColor("Exposure", 1, 1, PixelFormat::R32G32B32A32_Float));
	const FrameGraphTextureHandle sky = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("Sky", 1, 1, PixelFormat::R8G8B8A8_UNorm),
	    ResourceState::ShaderResource);

	resources.Transient.Scene = SceneRenderTargets{
	    .SceneColor = sceneColor,
	    .SceneDepth = sceneDepth,
	    .FinalSceneColor = finalSceneColor,
	    .BackBuffer = backBuffer};
	resources.Transient.Exposure = exposure;
	resources.External.Sky = sky;
	resources.External.Scene = DeclareRenderSceneGpuResources(builder);
	resources.History = DeclareFrameHistoryResources(builder, settings.RenderExtent);
	resources.ViewportProducts.SceneColor = sceneColor;
	resources.ViewportProducts.SceneDepth = sceneDepth;
	resources.ViewportProducts.Exposure = exposure;
}
