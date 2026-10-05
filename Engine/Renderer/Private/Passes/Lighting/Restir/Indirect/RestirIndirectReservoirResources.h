#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void CreateRestirIndirectHistoryResources(FrameGraphBuilder& builder, RenderViewportExtent extent, RenderFrameGraphResources& resources);

struct RestirIndirectWorkingReservoirs final
{
	FrameGraphTextureHandle TemporalSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle TemporalWeight = FrameGraphTextureHandle::Invalid();
};

RestirIndirectWorkingReservoirs CreateRestirIndirectWorkingReservoirs(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent);
