#include "../../../PCH.h"
#include "Passes/Presentation/Display/DisplayMapping.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Presentation/PresentationPolicy.h"
#include "Passes/Presentation/Display/ToneMapping.h"

FrameGraphTextureHandle AddDisplayMappingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent outputExtent,
    const RenderFrameGraphResources& resources)
{
	if (ResolveRenderViewPresentationDomain(builder.GetViewMode()) == RenderViewPresentationDomain::DisplayLinearExact)
	{
		return resources.Presentation.ResolvedSceneColor;
	}

	return AddToneMappingPass(builder, outputExtent, resources);
}
