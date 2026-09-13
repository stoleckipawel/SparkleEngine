#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerComposition.h"

#include "Frame/Graph/BuildRenderFrameGraph.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"
#include "Passes/PostProcessing/Exposure.h"
#include "Passes/Presentation/Upscaling.h"

void AddReferencePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources)
{
	resources.Transient.Scene.SceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ReferencePathTracer.RawRadiance",
	        settings.RenderExtent.Width,
	        settings.RenderExtent.Height,
	        PixelFormat::R32G32B32A32_Float));
	ReferencePathTracer::AddPass(builder, settings.RenderExtent, resources);
	AddExposurePass(builder, settings, resources);
	AddUpscalingPasses(builder, settings.RenderExtent, settings.OutputExtent, nullptr, resources);
	resources.ViewportProducts.SceneDepth = FrameGraphTextureHandle::Invalid();
}
