#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct UpscalerPassResources final
{
	FrameGraphTextureHandle InputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle OutputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Depth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVectors = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
};

void AddUpscalerPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    const UpscalerPassResources& inputs);
