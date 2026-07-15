#pragma once

#include "Commands/RhiQueue.h"
#include "Frame/RhiFrameConstants.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

class VulkanRenderCommandList;
class VulkanRhi;
class VulkanGpuMemoryAllocator;
class VulkanDescriptorManager;
class VulkanDescriptorAllocator;

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
	VulkanRenderCommandList& BeginCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	RhiSubmissionToken SubmitCommandList(
	    VulkanRenderCommandList& commandList,
	    std::uint32_t frameIndex,
	    std::span<const RhiSubmissionToken> waitTokens = {},
	    VkSemaphore binaryWaitSemaphore = VK_NULL_HANDLE,
	    VkPipelineStageFlags binaryWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VkSemaphore binarySignalSemaphore = VK_NULL_HANDLE) noexcept;
	void CancelFrame(std::uint32_t frameIndex) noexcept;
	void ConfigureCommandLists(
	    VulkanGpuMemoryAllocator& memoryAllocator,
	    VulkanDescriptorManager& descriptorManager,
	    VulkanDescriptorAllocator& descriptorAllocator) noexcept;

	VulkanRenderCommandList& GetCommandList(std::uint32_t frameIndex) noexcept;
	VulkanRenderCommandList& GetCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	VkCommandBuffer GetCommandBuffer(std::uint32_t frameIndex) const noexcept;
	VkCommandBuffer GetCommandBuffer(ERhiQueueType queueType, std::uint32_t frameIndex) const noexcept;
	bool IsCommandBufferRecording(VkCommandBuffer commandBuffer) const noexcept;
	bool IsRecording(const VulkanRenderCommandList& commandList, std::uint32_t frameIndex) const noexcept;

  private:
	struct CommandSlot final
	{
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
		std::unique_ptr<VulkanRenderCommandList> CommandList;
		bool IsRecording = false;
	};

	struct QueueFrameState final
	{
		VkCommandPool CommandPool = VK_NULL_HANDLE;
		std::vector<std::unique_ptr<CommandSlot>> Slots;
		std::size_t NextSlot = 0;
		CommandSlot* CurrentSlot = nullptr;
		RhiSubmissionToken LastSubmission{};
	};

	struct FrameState final
	{
		std::array<QueueFrameState, RhiQueueTypeCount> Queues;
	};

	QueueFrameState& GetQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	const QueueFrameState& GetQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex) const noexcept;
	void CreateFrameState(std::uint32_t frameIndex);
	void DestroyFrameState(FrameState& frameState) noexcept;
	CommandSlot& GetOrCreateSlot(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	CommandSlot* FindSlot(VulkanRenderCommandList& commandList, std::uint32_t frameIndex) noexcept;
	const CommandSlot* FindSlot(const VulkanRenderCommandList& commandList, std::uint32_t frameIndex) const noexcept;
	void ConfigureCommandList(VulkanRenderCommandList& commandList) noexcept;
	void NameQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex, QueueFrameState& frameState) noexcept;
	void NameCommandSlot(ERhiQueueType queueType, std::uint32_t frameIndex, std::size_t slotIndex, CommandSlot& slot) noexcept;

	VulkanRhi& m_rhi;
	std::array<FrameState, RhiFrameConstants::FramesInFlight> m_frames;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	VulkanDescriptorManager* m_descriptorManager = nullptr;
	VulkanDescriptorAllocator* m_descriptorAllocator = nullptr;
};
