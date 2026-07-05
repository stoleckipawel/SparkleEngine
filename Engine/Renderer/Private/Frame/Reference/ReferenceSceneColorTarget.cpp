#include "../../PCH.h"
#include "Frame/Reference/ReferenceSceneColorTarget.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/ResourceUsage.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
namespace
{
	constexpr const char* kReferenceTargetClearPassName = "ReferenceTargetClear";

	FrameGraphTextureHandle CreateReferenceTexture(
	    FrameGraphBuilder& builder,
	    const char* name,
	    RenderViewportExtent sceneExtent,
	    PixelFormat format)
	{
		FrameGraphTextureDesc desc =
		    FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, format);
		desc.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
		return builder.CreateTexture(desc);
	}

}

FrameGraphTextureHandle CreateReferenceSceneColorTarget(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	return CreateReferenceTexture(builder, "ReferenceSceneColor", sceneExtent, FrameRenderFormats::SceneColor);
}

void AddReferenceSceneColorClearPass(FrameGraphBuilder& builder, FrameGraphTextureHandle referenceSceneColor)
{
	builder.AddPass(
	    kReferenceTargetClearPassName,
	    EFrameGraphPassFlags::Raster,
	    [referenceSceneColor](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Write(referenceSceneColor, ResourceUsage::RenderTarget, "ReferenceSceneColor");
	    },
	    [referenceSceneColor](PassExecutionContext& context)
	    {
		    context.Resources.ClearRenderTarget(context.Commands, referenceSceneColor);
	    });
}
