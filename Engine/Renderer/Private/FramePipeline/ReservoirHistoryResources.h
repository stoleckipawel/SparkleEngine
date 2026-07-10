#pragma once

#include "Frame/RhiFrameConstants.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"

#include <array>
#include <cstdint>

class FrameGraph;
class RhiResourceService;

struct ReservoirHistoryFrameResources final
{
	RhiOwnedResourceHandle Sample;
	RhiOwnedResourceHandle Weight;
	RhiOwnedResourceHandle Surface;
};

struct ReservoirHistoryResourceSet final
{
	std::array<ReservoirHistoryFrameResources, RhiFrameConstants::FramesInFlight> Frames = {};
	RenderViewportExtent Extent = {};
};

struct ReservoirHistoryDebugNames final
{
	const wchar_t* Sample = nullptr;
	const wchar_t* Weight = nullptr;
	const wchar_t* Surface = nullptr;
};

struct ReservoirHistoryFrameGraphHandles final
{
	FrameGraphTextureHandle PreviousSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle PreviousWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle PreviousSurface = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentSurface = FrameGraphTextureHandle::Invalid();
};

void EnsureReservoirHistoryResources(
    RhiResourceService& resourceService,
    RenderViewportExtent extent,
    const ReservoirHistoryDebugNames& debugNames,
    ReservoirHistoryResourceSet& resources);

void ReleaseReservoirHistoryResources(RhiResourceService& resourceService, ReservoirHistoryResourceSet& resources) noexcept;

bool BindReservoirHistoryResources(
    FrameGraph& frameGraph,
    std::uint32_t currentFrameIndex,
    const ReservoirHistoryFrameGraphHandles& handles,
    const ReservoirHistoryResourceSet& resources) noexcept;

bool HasReservoirHistoryResources(const ReservoirHistoryResourceSet& resources) noexcept;
