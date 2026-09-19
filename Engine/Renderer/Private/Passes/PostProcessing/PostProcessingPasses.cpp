#include "../../PCH.h"
#include "Passes/PostProcessing/PostProcessingPasses.h"

#include "Passes/Debug/DebugPasses.h"
#include "Passes/Presentation/PresentationPasses.h"

void AddPostProcessingPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources)
{
	AddDebugPasses(builder, settings.OutputExtent, resources);
	AddPresentationPasses(builder, settings, resources);
}
