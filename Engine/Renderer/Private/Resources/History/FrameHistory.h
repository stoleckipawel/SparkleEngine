#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct FrameGraphTextureHistoryHandles final
{
	FrameGraphTextureHandle Previous = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Current = FrameGraphTextureHandle::Invalid();

	bool IsValid() const noexcept { return Previous.IsValid() && Current.IsValid(); }
};

struct FrameGraphReservoirHistoryHandles final
{
	FrameGraphTextureHistoryHandles Sample;
	FrameGraphTextureHistoryHandles Weight;
	FrameGraphTextureHistoryHandles Surface;
};

struct FrameHistoryResourceLayout final
{
	FrameGraphTextureHistoryHandles Exposure = {};
	FrameGraphTextureHistoryHandles ReferenceLighting = {};
	FrameGraphReservoirHistoryHandles DirectLightReservoir = {};
	FrameGraphReservoirHistoryHandles RestirIndirectReservoir = {};
};

struct FrameHistoryValidity final
{
	bool Exposure = false;
	bool ReferenceLighting = false;
	bool DirectLightReservoir = false;
	bool RestirIndirectReservoir = false;
};

FrameHistoryResourceLayout DeclareFrameHistoryResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent);
