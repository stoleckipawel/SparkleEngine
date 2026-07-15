#pragma once

#include "Commands/RhiQueue.h"
#include "Frame/RhiFrameConstants.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstdint>
#include <memory>
#include <span>

class VulkanRenderCommandList;
class VulkanRhi;

class VulkanCommandContext final
{
  public:
	explicit VulkanCommandContext(VulkanRhi& rhi);
	~VulkanCommandContext() noexcept;

	VulkanCommandContext(const VulkanCommandContext&) = delete;
	VulkanCommandContext& operator=(const VulkanCommandContext&) = delete;
	VulkanCommandContext(VulkanCommandContext&&) = delete;
	VulkanCommandContext& operator=(VulkanCommandContext&&) = delete;

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	void BeginCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	RhiSubmissionToken SubmitFrame(std::uint32_t frameIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) noexcept;
	RhiSubmissionToken SubmitCommandList(
	    ERhiQueueType queueType,
	    std::uint32_t frameIndex,
	    std::span<const RhiSubmissionToken> waitTokens = {},
	    VkSemaphore binaryWaitSemaphore = VK_NULL_HANDLE,
	    VkPipelineStageFlags binaryWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VkSemaphore binarySignalSemaphore = VK_NULL_HANDLE) noexcept;
	void CancelFrame(std::uint32_t frameIndex) noexcept;
	void WaitForIdle() noexcept;
	void WaitForSubmission(RhiSubmissionToken token) noexcept;
	bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept;
	RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept;
	std::uint64_t GetCompletedSubmissionValue(ERhiQueueType queueType) const noexcept;

	VulkanRenderCommandList& GetCommandList(std::uint32_t frameIndex) noexcept;
	VulkanRenderCommandList& GetCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	VkCommandBuffer GetCommandBuffer(std::uint32_t frameIndex) const noexcept;
	VkCommandBuffer GetCommandBuffer(ERhiQueueType queueType, std::uint32_t frameIndex) const noexcept;
	bool IsCommandBufferRecording(VkCommandBuffer commandBuffer) const noexcept;
	VkSemaphore GetImageAvailableSemaphore(std::uint32_t frameIndex) const noexcept;
	VkSemaphore GetRenderFinishedSemaphore(std::uint32_t frameIndex) const noexcept;

  private:
	struct QueueFrameState final
	{
		VkCommandPool CommandPool = VK_NULL_HANDLE;
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
		RhiSubmissionToken SubmissionToken{};
		std::unique_ptr<VulkanRenderCommandList> CommandList;
		bool IsRecording = false;
	};

	struct FrameState final
	{
		std::array<QueueFrameState, RhiQueueTypeCount> Queues;
		VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
	};

	struct QueueState final
	{
		VkSemaphore TimelineSemaphore = VK_NULL_HANDLE;
		std::uint64_t NextSubmissionValue = 1;
		std::uint64_t LastSubmittedValue = 0;
	};

	QueueFrameState& GetQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	const QueueFrameState& GetQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex) const noexcept;
	void CreateQueueState(ERhiQueueType queueType);
	void DestroyQueueState(QueueState& queueState) noexcept;
	void CreateFrameState(std::uint32_t frameIndex);
	void DestroyFrameState(FrameState& frameState) noexcept;
	void NameQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex, QueueFrameState& frameState) noexcept;

	VulkanRhi& m_rhi;
	std::array<FrameState, RhiFrameConstants::FramesInFlight> m_frames;
	std::array<QueueState, RhiQueueTypeCount> m_queues;
};
