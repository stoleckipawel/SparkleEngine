#pragma once

#include "Resources/History/FrameHistory.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct FrameAssemblyResourceLayout;

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
    FrameAssemblyResourceLayout& resources);
