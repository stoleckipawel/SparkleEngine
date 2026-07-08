#include "../../PCH.h"
#include "Frame/PostProcessing/PostProcessing.h"

#include "Frame/PostProcessing/Exposure.h"
#include "Frame/Presentation/Upscaling.h"

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    FrameAssemblyResourceLayout& resources)
{
	const bool hasRealtimeProviderInputs =
	    resources.Transient.GBuffer.MotionVector.IsValid() && resources.Transient.Lighting.IndirectDiffuse.IsValid();
	if (hasRealtimeProviderInputs && !resources.FinalSceneColorProduced)
	{
		AddUpscalingPasses(builder, renderExtent, outputExtent, resources);
	}

	AddExposurePass(
	    builder,
	    outputExtent,
	    resources.Transient.Scene.FinalSceneColor,
	    resources.History.PreviousExposure,
	    resources.History.CurrentExposure,
	    resources.Transient.Exposure);
}
