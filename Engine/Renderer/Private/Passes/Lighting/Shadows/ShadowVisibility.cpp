#include "../../../PCH.h"
#include "Passes/Lighting/Shadows/ShadowVisibility.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

DirectShadowSignalResources CreateDirectShadowSignalResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	resources.Transient.ShadowVisibilitySignal = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ShadowVisibilitySignalRaw",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        PixelFormat::R32G32B32A32_Float));
	resources.Transient.DirectLightTemporalReservoirSample = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "DirectLightTemporalReservoirSample",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        PixelFormat::R32G32B32A32_Float));
	resources.Transient.DirectLightTemporalReservoirWeight = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "DirectLightTemporalReservoirWeight",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        PixelFormat::R32G32B32A32_Float));
	return DirectShadowSignalResources{
	    .Visibility = resources.Transient.ShadowVisibilitySignal,
	    .TemporalReservoirSample = resources.Transient.DirectLightTemporalReservoirSample,
	    .TemporalReservoirWeight = resources.Transient.DirectLightTemporalReservoirWeight,
	    .ReservoirHistory = resources.History.DirectLightReservoir};
}
