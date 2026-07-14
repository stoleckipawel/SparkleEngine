#pragma once

#include "Resources/History/FrameHistory.h"
#include "Resources/History/PersistentReservoirHistory.h"

#include <cstdint>

class FrameGraph;
class RenderHardwareInterface;

struct FrameHistoryRequirements final
{
	RenderViewportExtent RenderExtent = {};
	bool ReferenceLighting = false;
	bool RestirLighting = false;
};

class PersistentFrameHistory final
{
  public:
	explicit PersistentFrameHistory(RenderHardwareInterface& renderHardwareInterface);

	void SetGraphLayout(const FrameHistoryResourceLayout& layout) noexcept;
	void Configure(const FrameHistoryRequirements& requirements);
	void SetReferenceLightingSemanticKey(std::uint64_t key) noexcept;
	bool SetRestirLightingSemanticKey(std::uint64_t key) noexcept;
	void Bind(FrameGraph& frameGraph, std::uint32_t frameIndex, std::uint64_t resetGeneration) noexcept;
	void CommitFrame() noexcept;
	FrameHistoryValidity GetValidity() const noexcept;

  private:
	PersistentTextureHistory m_exposure;
	PersistentTextureHistory m_referenceLighting;
	PersistentReservoirHistory m_directLightReservoir;
	PersistentReservoirHistory m_restirIndirectReservoir;
};
