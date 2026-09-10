#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerComposition.h"

#include "Frame/Graph/BuildRenderFrameGraph.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/Exposure.h"
#include "Passes/Presentation/Upscaling.h"
#include "Passes/Utility/ComputeClear.h"

void AddReferencePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources)
{
	AddComputeClearPass(builder, "ReferencePathTracer.Unavailable", resources.Transient.Scene.SceneColor, settings.RenderExtent);
	AddExposurePass(builder, settings, resources);
	AddUpscalingPasses(builder, settings.RenderExtent, settings.OutputExtent, nullptr, resources);
	resources.ViewportProducts.SceneDepth = FrameGraphTextureHandle::Invalid();
}
