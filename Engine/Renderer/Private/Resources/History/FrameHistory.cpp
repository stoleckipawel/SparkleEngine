#include "PCH.h"
#include "Resources/History/FrameHistory.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <string>

class ReservoirFrameHistory final
{
  public:
	static FrameGraphReservoirHistoryHandles DeclareReservoirHistory(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent extent,
	    std::string_view name)
	{
		const auto declare = [&](std::string_view suffix, PixelFormat format)
		{
			return builder.CreateTextureHistory(
			    FrameGraphTextureDesc::CreateColor(std::string(name) + std::string(suffix), extent.Width, extent.Height, format));
		};
		return FrameGraphReservoirHistoryHandles{
		    .Sample = declare("Sample", PixelFormat::R32G32B32A32_Float),
		    .Weight = declare("Weight", PixelFormat::R32G32B32A32_Float),
		    .Surface = declare("Surface", PixelFormat::R16G16B16A16_Float)};
	}

	static void InvalidateReservoir(FrameGraph& frameGraph, const FrameGraphReservoirHistoryHandles& history) noexcept
	{
		frameGraph.InvalidateTextureHistory(history.Sample);
		frameGraph.InvalidateTextureHistory(history.Weight);
		frameGraph.InvalidateTextureHistory(history.Surface);
	}

	static bool IsReservoirValid(const FrameGraph& frameGraph, const FrameGraphReservoirHistoryHandles& history) noexcept
	{
		return frameGraph.IsTextureHistoryValid(history.Sample) && frameGraph.IsTextureHistoryValid(history.Weight) &&
		       frameGraph.IsTextureHistoryValid(history.Surface);
	}
};

FrameHistoryResourceLayout DeclareFrameHistoryResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent)
{
	return FrameHistoryResourceLayout{
	    .Exposure = builder.CreateTextureHistory(
	        FrameGraphTextureDesc::CreateColor("Exposure", 1u, 1u, PixelFormat::R32G32B32A32_Float)),
	    .ReferenceLighting = builder.CreateTextureHistory(
	        FrameGraphTextureDesc::CreateColor(
	            "ReferenceLighting", renderExtent.Width, renderExtent.Height, PixelFormat::R32G32B32A32_Float)),
	    .DirectLightReservoir =
	        ReservoirFrameHistory::DeclareReservoirHistory(
	            builder,
	            renderExtent,
	            "DirectLightReservoir"),
	    .RestirIndirectReservoir =
	        ReservoirFrameHistory::DeclareReservoirHistory(
	            builder,
	            renderExtent,
	            "RestirIndirectReservoir")};
}

void InvalidateFrameHistory(FrameGraph& frameGraph, const FrameHistoryResourceLayout& history) noexcept
{
	frameGraph.InvalidateTextureHistory(history.Exposure);
	frameGraph.InvalidateTextureHistory(history.ReferenceLighting);
	InvalidateRestirLightingHistory(frameGraph, history);
}

void InvalidateRestirLightingHistory(FrameGraph& frameGraph, const FrameHistoryResourceLayout& history) noexcept
{
	ReservoirFrameHistory::InvalidateReservoir(
	    frameGraph,
	    history.DirectLightReservoir);
	ReservoirFrameHistory::InvalidateReservoir(
	    frameGraph,
	    history.RestirIndirectReservoir);
}

FrameHistoryValidity ResolveFrameHistoryValidity(
    const FrameGraph& frameGraph,
    const FrameHistoryResourceLayout& history) noexcept
{
	return FrameHistoryValidity{
	    .Exposure = frameGraph.IsTextureHistoryValid(history.Exposure),
	    .ReferenceLighting = frameGraph.IsTextureHistoryValid(history.ReferenceLighting),
	    .DirectLightReservoir =
	        ReservoirFrameHistory::IsReservoirValid(
	            frameGraph,
	            history.DirectLightReservoir),
	    .RestirIndirectReservoir =
	        ReservoirFrameHistory::IsReservoirValid(
	            frameGraph,
	            history.RestirIndirectReservoir)};
}
