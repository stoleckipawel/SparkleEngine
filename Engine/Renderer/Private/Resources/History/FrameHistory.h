#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHistory.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;
class FrameGraphBuilder;

struct FrameGraphReservoirHistoryHandles final
{
	FrameGraphTextureHistory Sample;
	FrameGraphTextureHistory Weight;
	FrameGraphTextureHistory Surface;
};

struct FrameHistoryResourceLayout final
{
	FrameGraphTextureHistory Exposure = {};
	FrameGraphTextureHistory ReferenceLighting = {};
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

void InvalidateFrameHistory(FrameGraph& frameGraph, const FrameHistoryResourceLayout& history) noexcept;
void InvalidateRestirLightingHistory(FrameGraph& frameGraph, const FrameHistoryResourceLayout& history) noexcept;
FrameHistoryValidity ResolveFrameHistoryValidity(
    const FrameGraph& frameGraph,
    const FrameHistoryResourceLayout& history) noexcept;
