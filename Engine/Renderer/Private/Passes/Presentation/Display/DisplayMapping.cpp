#include "../../../PCH.h"
#include "Passes/Presentation/Display/DisplayMapping.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Presentation/PresentationPolicy.h"
#include "Passes/Presentation/Display/ToneMapping.h"

FrameGraphTextureHandle AddDisplayMappingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent outputExtent,
    RenderViewMode viewMode,
    const RenderFrameGraphResources& resources)
{
	if (ResolveRenderViewPresentationDomain(viewMode) == RenderViewPresentationDomain::DisplayLinearExact)
	{
		return resources.Presentation.ResolvedSceneColor;
	}

	return AddToneMappingPass(builder, outputExtent, resources);
}
