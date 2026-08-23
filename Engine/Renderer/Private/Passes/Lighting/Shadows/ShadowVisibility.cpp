#include "../../../PCH.h"
#include "Passes/Lighting/Shadows/ShadowVisibility.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "FrameGraph/ResourceUsage.h"
#include "RHI/Public/Formats/PixelFormat.h"

DirectShadowSignalResources CreateDirectShadowSignalResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	FrameGraphTextureDesc visibilityDesc = FrameGraphTextureDesc::CreateColor(
	    "ShadowVisibilitySignalRaw",
	    sceneExtent.Width,
	    sceneExtent.Height,
	    PixelFormat::R32G32B32A32_Float);
	visibilityDesc.clearColor = {1.0f, 0.0f, 1.0f, 0.0f};
	resources.Transient.ShadowVisibilitySignal = builder.CreateTexture(visibilityDesc);
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

void AddShadowVisibilityFallbackPass(FrameGraphBuilder& builder, FrameGraphTextureHandle visibility)
{
	builder.AddPass(
	    "ShadowVisibilityFallback",
	    EFrameGraphPassKind::Raster,
	    [visibility](PassResourceBuilder& resourceBuilder)
	    { resourceBuilder.Write(visibility, ResourceUsage::RenderTarget, "ShadowVisibilitySignal"); },
	    [visibility](PassCommandContext& context) { context.Resources.ClearRenderTarget(context.Commands, visibility); });
}
