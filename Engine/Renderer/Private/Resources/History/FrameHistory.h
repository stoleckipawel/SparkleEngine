#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHistory.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;
class FrameGraphBuilder;
class RendererImageProviderStack;
class RenderViewState;
enum class LightingMode : std::uint8_t;
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
	FrameGraphTextureHistory ReferenceLighting = {};
	FrameGraphReservoirHistoryHandles DirectLightReservoir = {};
	FrameGraphReservoirHistoryHandles RestirIndirectReservoir = {};
};

FrameHistoryResourceLayout DeclareFrameHistoryResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent);

void InvalidateFrameHistory(FrameGraph& frameGraph, const FrameHistoryResourceLayout& history) noexcept;
void InvalidateRestirLightingHistory(FrameGraph& frameGraph, const FrameHistoryResourceLayout& history) noexcept;
void UpdateFrameHistory(
    FrameGraph& frameGraph,
    const FrameHistoryResourceLayout& history,
    LightingMode lighting,
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    RenderViewState& viewState,
    RendererImageProviderStack& imageProviders);
