#pragma once

#include "Resources/History/FrameHistory.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

struct DirectShadowSignalResources final
{
	FrameGraphTextureHandle Visibility = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle TemporalReservoirSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle TemporalReservoirWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphReservoirHistoryHandles ReservoirHistory = {};
};

DirectShadowSignalResources CreateDirectShadowSignalResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);
void AddShadowVisibilityFallbackPass(FrameGraphBuilder& builder, FrameGraphTextureHandle visibility);
