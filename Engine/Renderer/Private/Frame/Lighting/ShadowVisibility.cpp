#include "../../PCH.h"
#include "Frame/Lighting/ShadowVisibility.h"

#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

DirectShadowSignalResources CreateDirectShadowSignalResources(
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
	resources.Transient.ShadowLightSample = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "DirectShadowLightSample",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        PixelFormat::R32G32B32A32_Float));
	return DirectShadowSignalResources{
	    .Visibility = resources.Transient.ShadowVisibilitySignal,
	    .LightSample = resources.Transient.ShadowLightSample};
}
