#pragma once

#include "Resources/History/PersistentTextureHistory.h"

#include <cstdint>

struct PersistentReservoirHistorySpec final
{
	std::string_view Name;
};

class PersistentReservoirHistory final
{
  public:
	PersistentReservoirHistory(RenderHardwareInterface& renderHardwareInterface, const PersistentReservoirHistorySpec& spec);

	static FrameGraphReservoirHistoryHandles Reserve(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent extent,
	    const PersistentReservoirHistorySpec& spec);

	void SetGraphHandles(const FrameGraphReservoirHistoryHandles& handles) noexcept;
	void Configure(bool active, RenderViewportExtent extent);
	bool SetSemanticKey(std::uint64_t key) noexcept;
	void Bind(FrameGraph& frameGraph, std::uint32_t frameIndex, std::uint64_t resetGeneration) noexcept;
	void CommitFrame() noexcept;
	bool IsValid() const noexcept;

  private:
	PersistentTextureHistory m_sample;
	PersistentTextureHistory m_weight;
	PersistentTextureHistory m_surface;
	std::uint64_t m_semanticKey = 0;
};
