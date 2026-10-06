#include "../../PCH.h"
#include "Passes/Presentation/PresentationPasses.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Presentation/PresentationPolicy.h"
#include "Passes/Presentation/Display/ToneMapping.h"
#include "Passes/Presentation/Display/OutputEncoding.h"
#include "Passes/Presentation/PresentationOutput.h"

void AddPresentationPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    const ViewportRenderRequest& viewport,
    RenderFrameGraphResources& resources)
{
	if (!CanPublishPresentationOutput(resources))
	{
		return;
	}
	const FrameGraphTextureHandle displayLinearColor =
	    ResolveRenderViewPresentationDomain(viewport.ViewMode) == RenderViewPresentationDomain::DisplayLinearExact
	    ? resources.Presentation.ResolvedSceneColor
	    : AddToneMappingPass(builder, settings.OutputExtent, resources);
	const FrameGraphTextureHandle encodedColor = AddOutputEncodingPass(builder, settings, displayLinearColor);
	AddPresentationOutputPass(builder, settings, encodedColor, resources);
}
