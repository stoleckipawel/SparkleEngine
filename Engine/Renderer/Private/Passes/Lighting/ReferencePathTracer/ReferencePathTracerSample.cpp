#include "../../../PCH.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerSample.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

FrameGraphTextureHandle CreateReferencePathTracerSample(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	FrameGraphTextureDesc desc = FrameGraphTextureDesc::CreateColor(
	    "ReferencePathTracerSample",
	    sceneExtent.Width,
	    sceneExtent.Height,
	    PixelFormat::R32G32B32A32_Float);
	desc.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
	return builder.CreateTexture(desc);
}
