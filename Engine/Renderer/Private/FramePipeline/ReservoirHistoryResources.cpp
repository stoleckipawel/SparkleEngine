#include "PCH.h"
#include "FramePipeline/ReservoirHistoryResources.h"

#include "FrameGraph/FrameGraph.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Memory/RhiMemoryTypes.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"
#include "RHI/Public/Resources/RhiResourceService.h"

namespace
{
	RhiTextureResourceDesc BuildReservoirHistoryTextureDesc(RenderViewportExtent extent, PixelFormat format) noexcept
	{
		return RhiTextureResourceDesc{
		    .Width = extent.Width,
		    .Height = extent.Height,
		    .Format = format,
		    .MipLevels = 1u,
		    .AllowRenderTarget = false,
		    .AllowDepthStencil = false,
		    .AllowUnorderedAccess = true};
	}

	RhiOwnedResourceHandle CreateHistoryTexture(
	    RhiResourceService& resourceService,
	    const RhiTextureResourceDesc& desc,
	    const wchar_t* debugName)
	{
		return resourceService.CreateTextureResource(
		    desc,
		    ResourceState::Undefined,
		    RhiMemoryCategory::Texture,
		    RhiMemoryResidencyClass::DeviceLocal,
		    debugName);
	}

	void ClearBindings(FrameGraph& frameGraph, const ReservoirHistoryFrameGraphHandles& handles) noexcept
	{
		frameGraph.ClearPersistentTextureBinding(handles.PreviousSample);
		frameGraph.ClearPersistentTextureBinding(handles.PreviousWeight);
		frameGraph.ClearPersistentTextureBinding(handles.PreviousSurface);
		frameGraph.ClearPersistentTextureBinding(handles.CurrentSample);
		frameGraph.ClearPersistentTextureBinding(handles.CurrentWeight);
		frameGraph.ClearPersistentTextureBinding(handles.CurrentSurface);
	}
}

void EnsureReservoirHistoryResources(
    RhiResourceService& resourceService,
    RenderViewportExtent extent,
    const ReservoirHistoryDebugNames& debugNames,
    ReservoirHistoryResourceSet& resources)
{
	if (resources.Extent.Width != extent.Width || resources.Extent.Height != extent.Height)
	{
		ReleaseReservoirHistoryResources(resourceService, resources);
	}
	resources.Extent = extent;

	const RhiTextureResourceDesc sampleDesc = BuildReservoirHistoryTextureDesc(extent, PixelFormat::R32G32B32A32_Float);
	const RhiTextureResourceDesc surfaceDesc = BuildReservoirHistoryTextureDesc(extent, PixelFormat::R16G16B16A16_Float);
	for (ReservoirHistoryFrameResources& frame : resources.Frames)
	{
		if (!frame.Sample)
		{
			frame.Sample = CreateHistoryTexture(resourceService, sampleDesc, debugNames.Sample);
		}
		if (!frame.Weight)
		{
			frame.Weight = CreateHistoryTexture(resourceService, sampleDesc, debugNames.Weight);
		}
		if (!frame.Surface)
		{
			frame.Surface = CreateHistoryTexture(resourceService, surfaceDesc, debugNames.Surface);
		}
	}
}

void ReleaseReservoirHistoryResources(RhiResourceService& resourceService, ReservoirHistoryResourceSet& resources) noexcept
{
	for (ReservoirHistoryFrameResources& frame : resources.Frames)
	{
		for (RhiOwnedResourceHandle* resource : {&frame.Sample, &frame.Weight, &frame.Surface})
		{
			if (*resource)
			{
				resourceService.ReleaseOwnedResource(*resource);
				*resource = {};
			}
		}
	}
	resources.Extent = {};
}

bool BindReservoirHistoryResources(
    FrameGraph& frameGraph,
    std::uint32_t currentFrameIndex,
    const ReservoirHistoryFrameGraphHandles& handles,
    const ReservoirHistoryResourceSet& resources,
    ResourceState currentState) noexcept
{
	const std::uint32_t previousFrameIndex =
	    (currentFrameIndex + RhiFrameConstants::FramesInFlight - 1u) % RhiFrameConstants::FramesInFlight;
	const ReservoirHistoryFrameResources& previous = resources.Frames[previousFrameIndex];
	const ReservoirHistoryFrameResources& current = resources.Frames[currentFrameIndex];
	if (!previous.Sample || !previous.Weight || !previous.Surface || !current.Sample || !current.Weight || !current.Surface)
	{
		ClearBindings(frameGraph, handles);
		return false;
	}

	frameGraph.BindPersistentTexture(handles.PreviousSample, previous.Sample, currentState);
	frameGraph.BindPersistentTexture(handles.PreviousWeight, previous.Weight, currentState);
	frameGraph.BindPersistentTexture(handles.PreviousSurface, previous.Surface, currentState);
	frameGraph.BindPersistentTexture(handles.CurrentSample, current.Sample, currentState);
	frameGraph.BindPersistentTexture(handles.CurrentWeight, current.Weight, currentState);
	frameGraph.BindPersistentTexture(handles.CurrentSurface, current.Surface, currentState);
	return true;
}

bool HasReservoirHistoryResources(const ReservoirHistoryResourceSet& resources) noexcept
{
	for (const ReservoirHistoryFrameResources& frame : resources.Frames)
	{
		if (!frame.Sample || !frame.Weight || !frame.Surface)
		{
			return false;
		}
	}
	return true;
}
