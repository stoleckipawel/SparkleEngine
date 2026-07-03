#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct FrameAssemblyResourceLayout;

struct DirectShadowSignalResources final
{
	FrameGraphTextureHandle Visibility = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle TemporalReservoirSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle TemporalReservoirWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle PreviousReservoirSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle PreviousReservoirWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle PreviousReservoirSurface = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentReservoirSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentReservoirWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentReservoirSurface = FrameGraphTextureHandle::Invalid();
};

DirectShadowSignalResources CreateDirectShadowSignalResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources);
