#pragma once

#include "FrameGraph/FrameGraphTextureHistory.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include <string_view>

class FrameGraph;
class FrameGraphBuilder;
class RendererImageProviderStack;
class RenderViewState;
struct PreparedRenderScene;
struct RenderView;

struct FrameGraphReservoirHistoryHandles final
{
	FrameGraphTextureHistory Sample;
	FrameGraphTextureHistory Weight;
	FrameGraphTextureHistory Surface;
};

struct FrameHistoryResourceLayout final
{
	FrameGraphTextureHistory Exposure = {};
	FrameGraphReservoirHistoryHandles DirectLightReservoir = {};
	FrameGraphReservoirHistoryHandles RestirIndirectReservoir = {};
};

FrameHistoryResourceLayout DeclareFrameHistoryResources(FrameGraphBuilder& builder);
FrameGraphReservoirHistoryHandles DeclareLightingReservoirHistory(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    std::string_view name);

void InvalidateFrameHistory(FrameGraph& frameGraph, const FrameHistoryResourceLayout& history) noexcept;
void InvalidateRestirLightingHistory(FrameGraph& frameGraph, const FrameHistoryResourceLayout& history) noexcept;
void UpdateFrameHistory(
    FrameGraph& frameGraph,
    const FrameHistoryResourceLayout& history,
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    RenderViewState& viewState,
    RendererImageProviderStack& imageProviders);
