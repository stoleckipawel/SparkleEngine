#include "../../PCH.h"
#include "Passes/PostProcessing/PostProcessing.h"

#include "Passes/Debug/Debug.h"
#include "Passes/Presentation/Presentation.h"
#include "Passes/Presentation/Upscaling.h"

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    IUpscalerProvider* upscalerProvider,
    RenderFrameGraphResources& resources)
{
	if (!resources.ResolvedSceneColor.IsValid())
	{
		AddUpscalingPasses(builder, settings.RenderExtent, settings.OutputExtent, upscalerProvider, resources);
	}

	AddDebugPasses(builder, settings.OutputExtent, resources);

	AddPresentationPasses(builder, settings, resources);
}
