#include "PCH.h"
#include "Frame/Graph/ViewportFrameProductExports.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

void ExportViewportFrameProducts(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    const RenderFrameGraphResources& resources) noexcept
{
	if (resources.ViewportProducts.FinalColorLdr.IsValid())
	{
		builder.ExportTexture(resources.ViewportProducts.FinalColorLdr, "Viewport.FinalColorLdr");
	}

	if (HasAnyRenderOutputFlags(settings.RequestedOutputs, RenderOutputFlags::SceneDepth)
	    && resources.ViewportProducts.SceneDepth.IsValid())
	{
		builder.ExportTexture(resources.ViewportProducts.SceneDepth, "Viewport.SceneDepth");
	}

	if (HasAnyRenderOutputFlags(settings.RequestedOutputs, RenderOutputFlags::Normals) && resources.ViewportProducts.Normals.IsValid())
	{
		builder.ExportTexture(resources.ViewportProducts.Normals, "Viewport.Normals");
	}
}
