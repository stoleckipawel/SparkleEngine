#pragma once

#include "RHI/Public/Commands/RhiQueue.h"

#include <cstdint>
#include <vector>

class RhiCommandSubmissionService;

class GpuSceneSlotAllocator final
{
  public:
	explicit GpuSceneSlotAllocator(
	    RhiCommandSubmissionService* submissionService) noexcept;

	std::uint32_t Allocate();
	void Retire(std::uint32_t slot);

  private:
	struct RetiredSlot final
	{
		std::uint32_t Slot = 0;
		RhiSubmissionState LastUse;
	};

	void DrainCompletedRetirements();
	bool IsComplete(const RetiredSlot& retired) const noexcept;

	RhiCommandSubmissionService* m_submissionService = nullptr;
	std::vector<std::uint32_t> m_availableSlots;
	std::vector<RetiredSlot> m_retiredSlots;
	std::uint32_t m_nextSlot = 0;
};
