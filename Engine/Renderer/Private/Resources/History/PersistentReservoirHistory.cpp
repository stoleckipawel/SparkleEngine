#include "PCH.h"
#include "Resources/History/PersistentReservoirHistory.h"

PersistentReservoirHistory::PersistentReservoirHistory(
    RenderHardwareInterface& renderHardwareInterface,
    const PersistentReservoirHistorySpec& spec) :
	m_sample(renderHardwareInterface, std::string(spec.Name) + "Sample", PixelFormat::R32G32B32A32_Float),
	m_weight(renderHardwareInterface, std::string(spec.Name) + "Weight", PixelFormat::R32G32B32A32_Float),
	m_surface(renderHardwareInterface, std::string(spec.Name) + "Surface", PixelFormat::R16G16B16A16_Float)
{
}

FrameGraphReservoirHistoryHandles PersistentReservoirHistory::Reserve(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const PersistentReservoirHistorySpec& spec)
{
	return FrameGraphReservoirHistoryHandles{
	    .Sample = PersistentTextureHistory::Reserve(
	        builder,
	        extent,
	        std::string(spec.Name) + "Sample",
	        PixelFormat::R32G32B32A32_Float),
	    .Weight = PersistentTextureHistory::Reserve(
	        builder,
	        extent,
	        std::string(spec.Name) + "Weight",
	        PixelFormat::R32G32B32A32_Float),
	    .Surface = PersistentTextureHistory::Reserve(
	        builder,
	        extent,
	        std::string(spec.Name) + "Surface",
	        PixelFormat::R16G16B16A16_Float)};
}

void PersistentReservoirHistory::SetGraphHandles(const FrameGraphReservoirHistoryHandles& handles) noexcept
{
	m_sample.SetGraphHandles(handles.Sample);
	m_weight.SetGraphHandles(handles.Weight);
	m_surface.SetGraphHandles(handles.Surface);
}

void PersistentReservoirHistory::Configure(bool active, RenderViewportExtent extent)
{
	const bool enabled = active && extent.IsValid();
	if (!enabled)
	{
		m_semanticKey = 0;
	}
	m_sample.Configure(enabled, extent);
	m_weight.Configure(enabled, extent);
	m_surface.Configure(enabled, extent);
}

bool PersistentReservoirHistory::SetSemanticKey(std::uint64_t key) noexcept
{
	const bool changed = m_semanticKey != 0 && m_semanticKey != key;
	m_semanticKey = key;
	if (changed)
	{
		m_sample.Invalidate();
		m_weight.Invalidate();
		m_surface.Invalidate();
	}
	return changed;
}

void PersistentReservoirHistory::Bind(
    FrameGraph& frameGraph,
    std::uint32_t frameIndex,
    std::uint64_t resetGeneration) noexcept
{
	m_sample.Bind(frameGraph, frameIndex, resetGeneration);
	m_weight.Bind(frameGraph, frameIndex, resetGeneration);
	m_surface.Bind(frameGraph, frameIndex, resetGeneration);
}

void PersistentReservoirHistory::CommitFrame() noexcept
{
	m_sample.CommitFrame();
	m_weight.CommitFrame();
	m_surface.CommitFrame();
}

bool PersistentReservoirHistory::IsValid() const noexcept
{
	return m_sample.IsValid() && m_weight.IsValid() && m_surface.IsValid();
}
