#include "../../PCH.h"
#include "Frame/PostProcessing/PostProcessing.h"

#include "Frame/Core/FrameProviderInputs.h"
#include "Frame/PostProcessing/Exposure.h"
#include "Frame/Presentation/Upscaling.h"

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources)
{
	const bool hasRealtimeProviderInputs =
	    resources.Transient.GBuffer.MotionVector.IsValid() && resources.Transient.Lighting.IndirectDiffuse.IsValid();
	if (hasRealtimeProviderInputs && !resources.FinalSceneColorProduced)
	{
		resources.UpscalerProviderInputs =
		    BuildFrameUpscalerProviderInputs(resources.Transient.Scene, resources.Transient.GBuffer, resources.Transient.Exposure);
		AddUpscalerEvaluationPass(builder, sceneExtent, resources.Transient.Scene, resources.Transient.GBuffer);
		resources.FinalSceneColorProduced = true;
	}

	AddExposurePass(
	    builder,
	    sceneExtent,
	    resources.Transient.Scene.FinalSceneColor,
	    resources.History.PreviousExposure,
	    resources.History.CurrentExposure,
	    resources.Transient.Exposure);
}
