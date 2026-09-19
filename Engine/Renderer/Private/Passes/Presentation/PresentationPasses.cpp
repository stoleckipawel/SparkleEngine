#include "../../PCH.h"
#include "Passes/Presentation/PresentationPasses.h"

#include "Passes/Presentation/Display/DisplayMapping.h"
#include "Passes/Presentation/Display/OutputEncoding.h"
#include "Passes/Presentation/PresentationOutput.h"

void AddPresentationPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RenderFrameGraphResources& resources)
{
	const FrameGraphTextureHandle displayLinearColor = AddDisplayMappingPass(builder, settings.OutputExtent, viewMode, resources);
	const FrameGraphTextureHandle encodedColor = AddOutputEncodingPass(builder, settings, displayLinearColor);
	AddPresentationOutputPass(builder, settings, encodedColor, resources);
}
