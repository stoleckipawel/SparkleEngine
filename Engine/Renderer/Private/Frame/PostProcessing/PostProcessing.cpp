#include "../../PCH.h"
#include "Frame/PostProcessing/PostProcessing.h"

#include "Frame/Presentation/Upscaling.h"

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources)
{
	AddUpscalerEvaluationPass(builder, sceneExtent, resources.Transient.Scene, resources.Transient.GBuffer);
	resources.ProviderInputs = CreateUpscalerProviderInputs(resources.Transient.Scene, resources.Transient.GBuffer);
}
