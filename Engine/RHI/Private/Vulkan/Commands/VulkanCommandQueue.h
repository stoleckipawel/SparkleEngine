#pragma once

#include "Commands/RhiQueue.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <span>

class VulkanRhi;

struct VulkanQueueSubmission final
{
	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
	std::span<const RhiSubmissionToken> WaitTokens;
	VkSemaphore BinaryWaitSemaphore = VK_NULL_HANDLE;
	VkPipelineStageFlags BinaryWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSemaphore BinarySignalSemaphore = VK_NULL_HANDLE;
};

class VulkanCommandQueue final
{
  public:
	VulkanCommandQueue(VulkanRhi& rhi, ERhiQueueType queueType, VkQueue nativeQueue) noexcept;
	~VulkanCommandQueue() noexcept;

	VulkanCommandQueue(const VulkanCommandQueue&) = delete;
	VulkanCommandQueue& operator=(const VulkanCommandQueue&) = delete;
	VulkanCommandQueue(VulkanCommandQueue&&) = delete;
	VulkanCommandQueue& operator=(VulkanCommandQueue&&) = delete;

	RhiSubmissionToken Submit(const VulkanQueueSubmission& submission) noexcept;
	void WaitForSubmission(std::uint64_t submissionValue) noexcept;
	bool IsSubmissionComplete(std::uint64_t submissionValue) const noexcept;

	RhiSubmissionToken GetLastSubmittedToken() const noexcept;
	std::uint64_t GetCompletedSubmissionValue() const noexcept;
	ERhiQueueType GetQueueType() const noexcept { return m_queueType; }
	VkQueue GetNativeQueue() const noexcept { return m_queue; }
	VkSemaphore GetTimelineSemaphore() const noexcept { return m_timelineSemaphore; }

  private:
	VulkanRhi& m_rhi;
	ERhiQueueType m_queueType = ERhiQueueType::Graphics;
	VkQueue m_queue = VK_NULL_HANDLE;
	VkSemaphore m_timelineSemaphore = VK_NULL_HANDLE;
	std::uint64_t m_nextSubmissionValue = 1;
	std::uint64_t m_lastSubmittedValue = 0;
};
