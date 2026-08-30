#pragma once

#include "Commands/RhiCommandRecordingLease.h"
#include "Core/Public/Threading/ThreadOwnership.h"
#include "Frame/RhiFrameConstants.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <thread>
#include <vector>

class RenderCommandList;
class VulkanDescriptorService;
class VulkanGpuMemoryAllocator;
class VulkanRecordingDescriptorPool;
class VulkanRecordingUploadPage;
class VulkanRenderCommandList;
class VulkanRhi;

class VulkanCommandRecordingContext final
{
public:
	static constexpr std::uint32_t MaximumContextsPerFrameQueue = 8;
	static constexpr std::uint64_t UploadPageCapacityInBytes = 256 * 1024;

	VulkanCommandRecordingContext(
	    VulkanRhi& rhi,
	    VulkanGpuMemoryAllocator& memoryAllocator,
	    VulkanDescriptorService& descriptorService,
	    std::uint32_t maximumFramesInFlight) noexcept;
	~VulkanCommandRecordingContext() noexcept;

	VulkanCommandRecordingContext(const VulkanCommandRecordingContext&) = delete;
	VulkanCommandRecordingContext& operator=(const VulkanCommandRecordingContext&) = delete;
	VulkanCommandRecordingContext(VulkanCommandRecordingContext&&) = delete;
	VulkanCommandRecordingContext& operator=(VulkanCommandRecordingContext&&) = delete;

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	RhiCommandRecordingLease Acquire(ERhiQueueType queueType, std::uint32_t frameIndex, RhiCommandRecordingOwner owner) noexcept;
	RhiSubmissionToken Submit(
	    RhiCommandRecordingLease&& lease,
	    std::span<const RhiSubmissionToken> waitTokens = {},
	    VkSemaphore binaryWaitSemaphore = VK_NULL_HANDLE,
	    VkPipelineStageFlags binaryWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VkSemaphore binarySignalSemaphore = VK_NULL_HANDLE) noexcept;
	RhiSubmissionToken SubmitBatch(
	    std::span<RhiCommandRecordingLease> leases,
	    std::span<const RhiSubmissionToken> waitTokens = {},
	    VkSemaphore binaryWaitSemaphore = VK_NULL_HANDLE,
	    VkPipelineStageFlags binaryWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VkSemaphore binarySignalSemaphore = VK_NULL_HANDLE) noexcept;
	RenderCommandList& BeginCurrentGraphicsCommandList(std::uint32_t frameIndex) noexcept;
	RhiCommandRecordingLease TakeCurrentGraphicsCommandRecordingLease(std::uint32_t frameIndex) noexcept;
	RhiSubmissionToken SubmitCurrentGraphicsCommandList(
	    std::uint32_t frameIndex,
	    std::span<const RhiSubmissionToken> waitTokens = {},
	    VkSemaphore binaryWaitSemaphore = VK_NULL_HANDLE,
	    VkPipelineStageFlags binaryWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VkSemaphore binarySignalSemaphore = VK_NULL_HANDLE) noexcept;
	void CancelFrame(std::uint32_t frameIndex) noexcept;

	RenderCommandList& GetCurrentCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	RenderCommandList* TryGetCurrentCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;

private:
	enum class SlotState : std::uint8_t
	{
		Available,
		Recording,
		Closed,
		Submitted,
		Discarded,
	};

	struct CommandSlot final
	{
		VulkanCommandRecordingContext* Owner = nullptr;
		VkCommandPool CommandPool = VK_NULL_HANDLE;
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
		std::unique_ptr<VulkanRenderCommandList> CommandList;
		std::unique_ptr<VulkanRecordingDescriptorPool> DescriptorPool;
		std::unique_ptr<VulkanRecordingUploadPage> UploadPage;
		RhiSubmissionToken RetirementToken = {};
		RhiCommandRecordingOwner RecordingOwner = {};
		std::thread::id RecordingThread;
		std::uint32_t FrameSlot = 0;
		std::uint32_t ContextIndex = 0;
		ERhiQueueType QueueType = ERhiQueueType::Graphics;
		SlotState State = SlotState::Available;
		bool RequiresPoolReset = false;
	};

	struct QueueFrameState final
	{
		std::vector<std::unique_ptr<CommandSlot>> Slots;
		std::optional<RhiCommandRecordingLease> CurrentLease;
	};

	QueueFrameState& GetQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	CommandSlot& AcquireSlot(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	void InitializeSlots();
	CommandSlot& CreateSlot(ERhiQueueType queueType, std::uint32_t frameIndex);
	void CreateCommandPool(CommandSlot& slot);
	void AllocateCommandBuffer(CommandSlot& slot);
	void InitializeRecordingResources(CommandSlot& slot);
	void NameSlotObjects(const CommandSlot& slot) const noexcept;
	void WaitForFrameStateRetirement(const QueueFrameState& frameState) noexcept;
	void ResetSlot(CommandSlot& slot) noexcept;
	void ResetCommandPool(CommandSlot& slot) noexcept;
	void BeginSlot(CommandSlot& slot) noexcept;
	void CloseSlot(CommandSlot& slot) noexcept;
	void ReleaseSlot(CommandSlot& slot) noexcept;
	CommandSlot* ConsumeClosedLease(RhiCommandRecordingLease&& lease) noexcept;
	void ResolveSubmittedSlot(CommandSlot& slot, RhiSubmissionToken token) noexcept;
	void DestroySlots() noexcept;

	static void BeginLease(void* state) noexcept;
	static void CloseLease(void* state) noexcept;
	static void ReleaseLease(void* state, bool closed) noexcept;
	[[noreturn]] void FailExhausted(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	[[noreturn]] static void FailOwnershipViolation(const CommandSlot& slot, const char* operation) noexcept;

	Threading::OwnerThread m_owner{"Vulkan command recording coordinator"};
	VulkanRhi* m_rhi = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	VulkanDescriptorService* m_descriptorService = nullptr;
	std::vector<std::array<QueueFrameState, RhiQueueTypeCount>> m_frames;
};
