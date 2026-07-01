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
	if (hasRealtimeProviderInputs)
	{
		AddUpscalerEvaluationPass(builder, sceneExtent, resources.Transient.Scene, resources.Transient.GBuffer);
	}

	AddExposurePass(
	    builder,
	    sceneExtent,
	    resources.Transient.Scene.FinalSceneColor,
	    resources.History.PreviousExposure,
	    resources.History.CurrentExposure,
	    resources.Transient.Exposure);

	if (hasRealtimeProviderInputs)
	{
		resources.UpscalerProviderInputs =
		    BuildFrameUpscalerProviderInputs(resources.Transient.Scene, resources.Transient.GBuffer, resources.Transient.Exposure);
		resources.DenoiserProviderInputs = BuildFrameDenoiserProviderInputs(
		    resources.Transient.Scene,
		    resources.Transient.GBuffer,
		    resources.Transient.Lighting,
		    resources.Transient.Exposure);
	}
}
