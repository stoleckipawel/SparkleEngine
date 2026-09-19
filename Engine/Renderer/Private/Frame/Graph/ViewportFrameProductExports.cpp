#include "PCH.h"
#include "Frame/Graph/ViewportFrameProductExports.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include <string_view>

static void ExportTextureIfValid(FrameGraphBuilder& builder, FrameGraphTextureHandle handle, std::string_view name) noexcept
{
	if (handle.IsValid())
	{
		builder.ExportTexture(handle, name);
	}
}

void ExportViewportFrameProducts(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    const RenderFrameGraphResources& resources) noexcept
{
	builder.ExportTexture(resources.ViewportProducts.FinalColorLdr, "Viewport.FinalColorLdr");

	if (HasAnyRenderOutputFlags(settings.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		ExportTextureIfValid(builder, resources.ViewportProducts.SceneDepth, "Viewport.SceneDepth");
	}

	if (HasAnyRenderOutputFlags(settings.RequestedOutputs, RenderOutputFlags::Normals))
	{
		ExportTextureIfValid(builder, resources.ViewportProducts.Normals, "Viewport.Normals");
	}
}
