#include "../../PCH.h"
#include "Frame/Lighting/ShadowVisibility.h"

#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

FrameGraphTextureHandle CreateShadowVisibilityResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources)
{
	resources.Transient.ShadowVisibilitySignal = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ShadowVisibilitySignalRaw",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        PixelFormat::R32G32B32A32_Float));
	return resources.Transient.ShadowVisibilitySignal;
}
