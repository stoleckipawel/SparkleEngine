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

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    const FrameBuildSettings& settings,
    IUpscalerProvider* upscalerProvider,
    FrameAssemblyResourceLayout& resources)
{
	if (!resources.FinalSceneColorProduced)
	{
		AddUpscalingPasses(builder, settings.RenderExtent, settings.OutputExtent, upscalerProvider, resources);
	}

	AddDebugPasses(builder, settings.OutputExtent, resources);

	AddPresentationPasses(builder, settings, resources);
}
