#include "PCH.h"
#include "Resources/History/PersistentFrameHistory.h"

namespace
{
	inline constexpr PersistentTextureHistorySpec ExposureHistory{
	    .Name = "Exposure",
	    .Format = PixelFormat::R32G32B32A32_Float};
	inline constexpr PersistentTextureHistorySpec ReferenceLightingHistory{
	    .Name = "ReferenceLighting",
	    .Format = PixelFormat::R32G32B32A32_Float};
	inline constexpr PersistentReservoirHistorySpec DirectLightReservoirHistory{
	    .Name = "DirectLightReservoir"};
	inline constexpr PersistentReservoirHistorySpec RestirIndirectReservoirHistory{
	    .Name = "RestirIndirectReservoir"};
}

FrameHistoryResourceLayout DeclareFrameHistoryResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent)
{
	return FrameHistoryResourceLayout{
	    .Exposure = PersistentTextureHistory::Reserve(builder, {1u, 1u}, ExposureHistory),
	    .ReferenceLighting = PersistentTextureHistory::Reserve(builder, renderExtent, ReferenceLightingHistory),
	    .DirectLightReservoir = PersistentReservoirHistory::Reserve(builder, renderExtent, DirectLightReservoirHistory),
	    .RestirIndirectReservoir = PersistentReservoirHistory::Reserve(builder, renderExtent, RestirIndirectReservoirHistory)};
}

PersistentFrameHistory::PersistentFrameHistory(RenderHardwareInterface& renderHardwareInterface) :
	m_exposure(renderHardwareInterface, ExposureHistory),
	m_referenceLighting(renderHardwareInterface, ReferenceLightingHistory),
	m_directLightReservoir(renderHardwareInterface, DirectLightReservoirHistory),
	m_restirIndirectReservoir(renderHardwareInterface, RestirIndirectReservoirHistory)
{
}

void PersistentFrameHistory::SetGraphLayout(const FrameHistoryResourceLayout& layout) noexcept
{
	m_exposure.SetGraphHandles(layout.Exposure);
	m_referenceLighting.SetGraphHandles(layout.ReferenceLighting);
	m_directLightReservoir.SetGraphHandles(layout.DirectLightReservoir);
	m_restirIndirectReservoir.SetGraphHandles(layout.RestirIndirectReservoir);
}

void PersistentFrameHistory::Configure(const FrameHistoryRequirements& requirements)
{
	m_exposure.Configure(true, {1u, 1u});
	m_referenceLighting.Configure(requirements.ReferenceLighting, requirements.RenderExtent);
	m_directLightReservoir.Configure(requirements.RestirLighting, requirements.RenderExtent);
	m_restirIndirectReservoir.Configure(requirements.RestirLighting, requirements.RenderExtent);
}

void PersistentFrameHistory::SetReferenceLightingSemanticKey(std::uint64_t key) noexcept
{
	m_referenceLighting.SetSemanticKey(key);
}

bool PersistentFrameHistory::SetRestirLightingSemanticKey(std::uint64_t key) noexcept
{
	const bool directChanged = m_directLightReservoir.SetSemanticKey(key);
	const bool indirectChanged = m_restirIndirectReservoir.SetSemanticKey(key);
	return directChanged || indirectChanged;
}

void PersistentFrameHistory::Bind(
    FrameGraph& frameGraph,
    std::uint32_t frameIndex,
    std::uint64_t resetGeneration) noexcept
{
	m_exposure.Bind(frameGraph, frameIndex, resetGeneration);
	m_referenceLighting.Bind(frameGraph, frameIndex, resetGeneration);
	m_directLightReservoir.Bind(frameGraph, frameIndex, resetGeneration);
	m_restirIndirectReservoir.Bind(frameGraph, frameIndex, resetGeneration);
}

void PersistentFrameHistory::CommitFrame() noexcept
{
	m_exposure.CommitFrame();
	m_referenceLighting.CommitFrame();
	m_directLightReservoir.CommitFrame();
	m_restirIndirectReservoir.CommitFrame();
}

FrameHistoryValidity PersistentFrameHistory::GetValidity() const noexcept
{
	return FrameHistoryValidity{
	    .Exposure = m_exposure.IsValid(),
	    .ReferenceLighting = m_referenceLighting.IsValid(),
	    .DirectLightReservoir = m_directLightReservoir.IsValid(),
	    .RestirIndirectReservoir = m_restirIndirectReservoir.IsValid()};
}
