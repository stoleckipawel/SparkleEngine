#include "../../PCH.h"
#include "Passes/PostProcessing/PostProcessing.h"

#include "Passes/Debug/Debug.h"
#include "Passes/Presentation/Presentation.h"

void AddPostProcessingPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources)
{
	AddDebugPasses(builder, settings.OutputExtent, resources);

	AddPresentationPasses(builder, settings, resources);
}
