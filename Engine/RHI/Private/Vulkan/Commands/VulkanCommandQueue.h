#pragma once

#include "Commands/RhiQueue.h"
#include "Core/Public/Threading/ThreadOwnership.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <span>

class VulkanRhi;

namespace spdlog
{
class logger;
}

struct VulkanQueueSubmission final
{
	std::span<const VkCommandBuffer> CommandBuffers;
	std::span<const RhiSubmissionToken> WaitTokens;
	VkSemaphore BinaryWaitSemaphore = VK_NULL_HANDLE;
	VkPipelineStageFlags BinaryWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSemaphore BinarySignalSemaphore = VK_NULL_HANDLE;
};

struct VulkanNativeQueue final
{
	VkQueue Queue = VK_NULL_HANDLE;
	std::mutex SubmissionMutex;
};

class VulkanCommandQueue final
{
  public:
	VulkanCommandQueue(
	    VulkanRhi& rhi,
	    ERhiQueueType queueType,
	    std::shared_ptr<VulkanNativeQueue> nativeQueue) noexcept;
	~VulkanCommandQueue() noexcept;

	VulkanCommandQueue(const VulkanCommandQueue&) = delete;
	VulkanCommandQueue& operator=(const VulkanCommandQueue&) = delete;
	VulkanCommandQueue(VulkanCommandQueue&&) = delete;
	VulkanCommandQueue& operator=(VulkanCommandQueue&&) = delete;

	RhiSubmissionToken Submit(const VulkanQueueSubmission& submission) noexcept;
	VkResult Present(const VkPresentInfoKHR& presentInfo) noexcept;
	void DrainForSwapChainRecreation() noexcept;
	void WaitForSubmission(std::uint64_t submissionValue) noexcept;
	bool HasSubmitted(std::uint64_t submissionValue) const noexcept;
	bool IsSubmissionComplete(std::uint64_t submissionValue) const noexcept;

	RhiSubmissionToken GetLastSubmittedToken() const noexcept;
	std::uint64_t GetCompletedSubmissionValue() const noexcept;
	ERhiQueueType GetQueueType() const noexcept { return m_queueType; }
	VkQueue GetNativeQueue() const noexcept;
	VkSemaphore GetTimelineSemaphore() const noexcept;

  private:
	struct NativeSubmission;

	static const std::shared_ptr<spdlog::logger>& GetLogger();
	static std::uint64_t GetWaitTimeoutNanoseconds() noexcept;
	bool ResolveWaitState(
	    std::span<const RhiSubmissionToken> waitTokens,
	    RhiSubmissionState& waitState) const noexcept;
	void BuildNativeSubmission(
	    const VulkanQueueSubmission& submission,
	    const RhiSubmissionState& waitState,
	    std::uint64_t submissionValue,
	    NativeSubmission& nativeSubmission) const noexcept;
	VkResult SubmitNative(const VkSubmitInfo& submission) noexcept;

	Threading::OwnerThread m_owner{"Vulkan command queue"};
	VulkanRhi& m_rhi;
	ERhiQueueType m_queueType = ERhiQueueType::Graphics;
	std::shared_ptr<VulkanNativeQueue> m_nativeQueue;
	VkSemaphore m_timelineSemaphore = VK_NULL_HANDLE;
	std::uint64_t m_nextSubmissionValue = 1;
	std::uint64_t m_lastSubmittedValue = 0;
};
