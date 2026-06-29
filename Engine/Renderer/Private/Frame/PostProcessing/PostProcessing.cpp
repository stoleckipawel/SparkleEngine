#include "../../PCH.h"
#include "Frame/PostProcessing/PostProcessing.h"

#include "Frame/PostProcessing/Exposure.h"
#include "Frame/Presentation/Upscaling.h"

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources)
{
	AddUpscalerEvaluationPass(builder, sceneExtent, resources.Transient.Scene, resources.Transient.GBuffer);
	AddExposurePass(
	    builder,
	    sceneExtent,
	    resources.Transient.Scene.FinalSceneColor,
	    resources.History.PreviousExposure,
	    resources.History.CurrentExposure,
	    resources.Transient.Exposure);
	resources.ProviderInputs = CreateUpscalerProviderInputs(resources.Transient.Scene, resources.Transient.GBuffer);
	resources.ProviderInputs.Exposure = resources.Transient.Exposure;
}
