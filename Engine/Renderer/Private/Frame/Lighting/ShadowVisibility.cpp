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
	    .PreviousReservoirSample = resources.History.PreviousDirectLightReservoirSample,
	    .PreviousReservoirWeight = resources.History.PreviousDirectLightReservoirWeight,
	    .PreviousReservoirSurface = resources.History.PreviousDirectLightReservoirSurface,
	    .CurrentReservoirSample = resources.History.CurrentDirectLightReservoirSample,
	    .CurrentReservoirWeight = resources.History.CurrentDirectLightReservoirWeight,
	    .CurrentReservoirSurface = resources.History.CurrentDirectLightReservoirSurface};
}
