#include "../../PCH.h"
#include "Frame/PostProcessing/PostProcessing.h"

#include "Frame/Debug/Debug.h"
#include "Frame/PostProcessing/Exposure.h"
#include "Frame/Presentation/Presentation.h"
#include "Frame/Presentation/Upscaling.h"

void AddPreReconstructionPostProcessingPasses(
    FrameGraphBuilder& builder,
    const FrameBuildSettings& settings,
    FrameAssemblyResourceLayout& resources)
{
	AddExposurePass(builder, settings, resources);
}

void AddPostProcessingPasses(FrameGraphBuilder& builder, const FrameBuildSettings& settings, FrameAssemblyResourceLayout& resources)
{
	if (!resources.FinalSceneColorProduced)
	{
		AddUpscalingPasses(builder, settings.RenderExtent, settings.OutputExtent, resources);
	}

	AddDebugPasses(builder, resources);

	AddPresentationPasses(builder, settings, resources);
}
