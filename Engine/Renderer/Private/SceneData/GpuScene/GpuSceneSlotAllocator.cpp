#include "PCH.h"
#include "SceneData/GpuScene/GpuSceneSlotAllocator.h"

#include "RHI/Public/Commands/RhiCommandSubmissionService.h"

#include <algorithm>
#include <array>

GpuSceneSlotAllocator::GpuSceneSlotAllocator(
    RhiCommandSubmissionService* submissionService) noexcept :
	m_submissionService(submissionService)
{
}

std::uint32_t GpuSceneSlotAllocator::Allocate()
{
	DrainCompletedRetirements();
	if (m_availableSlots.empty())
	{
		return m_nextSlot++;
	}

	const auto available =
	    std::min_element(
	        m_availableSlots.begin(),
	        m_availableSlots.end());
	const std::uint32_t slot = *available;
	m_availableSlots.erase(available);
	return slot;
}

void GpuSceneSlotAllocator::Retire(std::uint32_t slot)
{
	RetiredSlot retired{.Slot = slot};
	if (m_submissionService != nullptr)
	{
		for (std::size_t queueIndex = 0;
		     queueIndex < RhiQueueTypeCount;
		     ++queueIndex)
		{
			retired.LastUse.MarkUsed(
			    m_submissionService->GetLastSubmittedToken(
			        static_cast<ERhiQueueType>(queueIndex)));
		}
	}

	if (IsComplete(retired))
	{
		m_availableSlots.push_back(slot);
		return;
	}
	m_retiredSlots.push_back(retired);
}

void GpuSceneSlotAllocator::DrainCompletedRetirements()
{
	for (std::size_t index = 0;
	     index < m_retiredSlots.size();)
	{
		if (!IsComplete(m_retiredSlots[index]))
		{
			++index;
			continue;
		}
		m_availableSlots.push_back(
		    m_retiredSlots[index].Slot);
		m_retiredSlots.erase(
		    m_retiredSlots.begin() + index);
	}
}

bool GpuSceneSlotAllocator::IsComplete(
    const RetiredSlot& retired) const noexcept
{
	if (m_submissionService == nullptr)
	{
		return true;
	}

	std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens;
	const std::size_t tokenCount =
	    retired.LastUse.CopyTokens(tokens);
	for (std::size_t index = 0; index < tokenCount; ++index)
	{
		if (!m_submissionService->IsSubmissionComplete(
		        tokens[index]))
		{
			return false;
		}
	}
	return true;
}
