#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanCommandRecordingContext.h"

#include "Commands/RhiCommandRecordingLeaseAccess.h"
#include "Vulkan/Commands/VulkanCommandQueue.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorService.h"
#include "Vulkan/Descriptors/VulkanRecordingDescriptorPool.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Resources/VulkanRecordingUploadPage.h"

#include <cassert>
#include <exception>
#include <format>
#include <string>

static const auto g_vulkanCommandRecordingLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Commands");

VulkanCommandRecordingContext::VulkanCommandRecordingContext(
    VulkanRhi& rhi,
    VulkanGpuMemoryAllocator& memoryAllocator,
    VulkanDescriptorService& descriptorService,
    std::uint32_t maximumFramesInFlight) noexcept :
	m_rhi(&rhi),
	m_memoryAllocator(&memoryAllocator),
	m_descriptorService(&descriptorService),
	m_frames(maximumFramesInFlight)
{
	InitializeSlots();
}

VulkanCommandRecordingContext::~VulkanCommandRecordingContext() noexcept
{
	DestroySlots();
}

void VulkanCommandRecordingContext::BeginFrame(std::uint32_t frameIndex) noexcept
{
	m_owner.AssertAccess();
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		QueueFrameState& frameState = GetQueueFrameState(static_cast<ERhiQueueType>(queueIndex), frameIndex);
		frameState.CurrentLease.reset();

		for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
		{
			if (slot->State == SlotState::Recording ||
			    slot->State == SlotState::Closed)
			{
				FailOwnershipViolation(
				    *slot,
				    "begin a frame while a recording lease remains active");
			}
		}

		WaitForFrameStateRetirement(frameState);
		for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
		{
			if (slot->State != SlotState::Available)
			{
				ResetSlot(*slot);
			}
		}
	}
}

RhiCommandRecordingLease VulkanCommandRecordingContext::Acquire(
    ERhiQueueType queueType,
    std::uint32_t frameIndex,
    RhiCommandRecordingOwner owner) noexcept
{
	m_owner.AssertAccess();
	CommandSlot& slot = AcquireSlot(queueType, frameIndex);
	const RhiSubmissionToken reusableAfter = slot.RetirementToken;
	slot.RetirementToken = {};
	slot.RecordingOwner = owner;
	slot.RecordingThread = {};
	slot.State = SlotState::Recording;
	slot.CommandList->SetRecordingOwner(owner);

	const RhiCommandRecordingLeaseInitialization initialization{
	    .BackendState = &slot,
	    .CommandList = slot.CommandList.get(),
	    .QueueType = queueType,
	    .FrameSlot = slot.FrameSlot,
	    .ContextId = RhiCommandRecordingContextId{.Value = slot.ContextIndex},
	    .Owner = owner,
	    .UploadPage = RhiCommandRecordingUploadPage{.CapacityInBytes = slot.UploadPage->GetCapacityInBytes()},
	    .DescriptorPage = RhiCommandRecordingDescriptorPage{.Capacity = slot.DescriptorPool->GetCapacity()},
	    .RetirementToken = reusableAfter,
	    .Begin = &BeginLease,
	    .Close = &CloseLease,
	    .Release = &ReleaseLease};
	return RhiCommandRecordingLeaseAccess::Create(initialization);
}

RhiSubmissionToken VulkanCommandRecordingContext::Submit(
    RhiCommandRecordingLease&& lease,
    std::span<const RhiSubmissionToken> waitTokens,
    VkSemaphore binaryWaitSemaphore,
    VkPipelineStageFlags binaryWaitStage,
    VkSemaphore binarySignalSemaphore) noexcept
{
	std::array<RhiCommandRecordingLease, 1> leases;
	leases.front() = std::move(lease);
	return SubmitBatch(
	    leases,
	    waitTokens,
	    binaryWaitSemaphore,
	    binaryWaitStage,
	    binarySignalSemaphore);
}

RhiSubmissionToken VulkanCommandRecordingContext::SubmitBatch(
    std::span<RhiCommandRecordingLease> leases,
    std::span<const RhiSubmissionToken> waitTokens,
    VkSemaphore binaryWaitSemaphore,
    VkPipelineStageFlags binaryWaitStage,
    VkSemaphore binarySignalSemaphore) noexcept
{
	m_owner.AssertAccess();
	if (leases.empty() ||
	    leases.size() > MaximumContextsPerFrameQueue)
	{
		return {};
	}

	const ERhiQueueType queueType = leases.front().GetQueueType();
	std::array<CommandSlot*, MaximumContextsPerFrameQueue> slots{};
	std::array<VkCommandBuffer, MaximumContextsPerFrameQueue> commandBuffers{};

	for (std::size_t index = 0; index < leases.size(); ++index)
	{
		if (!leases[index].IsClosed())
		{
			leases[index].Close();
		}

		CommandSlot* const slot =
		    ConsumeClosedLease(std::move(leases[index]));
		if (slot == nullptr ||
		    slot->QueueType != queueType)
		{
			return {};
		}

		slots[index] = slot;
		commandBuffers[index] = slot->CommandBuffer;
	}

	const RhiSubmissionToken token = m_rhi->GetCommandQueue(queueType).Submit(
	    VulkanQueueSubmission{
	        .CommandBuffers =
	            std::span<const VkCommandBuffer>(
	                commandBuffers.data(),
	                leases.size()),
	        .WaitTokens = waitTokens,
	        .BinaryWaitSemaphore = binaryWaitSemaphore,
	        .BinaryWaitStage = binaryWaitStage,
	        .BinarySignalSemaphore = binarySignalSemaphore});

	for (std::size_t index = 0; index < leases.size(); ++index)
	{
		ResolveSubmittedSlot(*slots[index], token);
	}

	return token;
}

RenderCommandList& VulkanCommandRecordingContext::BeginCurrentGraphicsCommandList(
    std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(ERhiQueueType::Graphics, frameIndex);
	frameState.CurrentLease.emplace(Acquire(ERhiQueueType::Graphics, frameIndex, RhiCommandRecordingOwner{}));
	return frameState.CurrentLease->GetCommandList();
}

RhiCommandRecordingLease
VulkanCommandRecordingContext::TakeCurrentGraphicsCommandRecordingLease(
    std::uint32_t frameIndex) noexcept
{
	m_owner.AssertAccess();
	QueueFrameState& frameState =
	    GetQueueFrameState(ERhiQueueType::Graphics, frameIndex);
	if (!frameState.CurrentLease.has_value())
	{
		return {};
	}

	RhiCommandRecordingLease lease(
	    std::move(*frameState.CurrentLease));
	frameState.CurrentLease.reset();
	return lease;
}

RhiSubmissionToken VulkanCommandRecordingContext::SubmitCurrentGraphicsCommandList(
    std::uint32_t frameIndex,
    std::span<const RhiSubmissionToken> waitTokens,
    VkSemaphore binaryWaitSemaphore,
    VkPipelineStageFlags binaryWaitStage,
    VkSemaphore binarySignalSemaphore) noexcept
{
	RhiCommandRecordingLease lease =
	    TakeCurrentGraphicsCommandRecordingLease(frameIndex);
	if (!lease.IsValid())
	{
		return {};
	}

	return Submit(std::move(lease), waitTokens, binaryWaitSemaphore, binaryWaitStage, binarySignalSemaphore);
}

void VulkanCommandRecordingContext::CancelFrame(std::uint32_t frameIndex) noexcept
{
	m_owner.AssertAccess();
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		QueueFrameState& frameState = GetQueueFrameState(static_cast<ERhiQueueType>(queueIndex), frameIndex);
		frameState.CurrentLease.reset();
	}
}

RenderCommandList& VulkanCommandRecordingContext::GetCurrentCommandList(
    ERhiQueueType queueType,
    std::uint32_t frameIndex) noexcept
{
	m_owner.AssertAccess();
	RenderCommandList* const commandList = TryGetCurrentCommandList(queueType, frameIndex);
	assert(commandList != nullptr);
	return *commandList;
}

RenderCommandList* VulkanCommandRecordingContext::TryGetCurrentCommandList(
    ERhiQueueType queueType,
    std::uint32_t frameIndex) noexcept
{
	m_owner.AssertAccess();
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	return frameState.CurrentLease.has_value() ? &frameState.CurrentLease->GetCommandList() : nullptr;
}

VulkanCommandRecordingContext::QueueFrameState& VulkanCommandRecordingContext::GetQueueFrameState(
    ERhiQueueType queueType,
    std::uint32_t frameIndex) noexcept
{
	return m_frames[frameIndex % m_frames.size()][RhiQueueTypeToIndex(queueType)];
}

VulkanCommandRecordingContext::CommandSlot& VulkanCommandRecordingContext::AcquireSlot(
    ERhiQueueType queueType,
    std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
	{
		if (slot->State == SlotState::Available)
		{
			return *slot;
		}
	}

	FailExhausted(queueType, frameIndex);
}

void VulkanCommandRecordingContext::InitializeSlots()
{
	for (std::uint32_t frameIndex = 0;
	     frameIndex < m_frames.size();
	     ++frameIndex)
	{
		for (std::size_t queueIndex = 0;
		     queueIndex < RhiQueueTypeCount;
		     ++queueIndex)
		{
			const ERhiQueueType queueType =
			    static_cast<ERhiQueueType>(queueIndex);
			GetQueueFrameState(queueType, frameIndex)
			    .Slots.reserve(MaximumContextsPerFrameQueue);

			for (std::uint32_t contextIndex = 0;
			     contextIndex < MaximumContextsPerFrameQueue;
			     ++contextIndex)
			{
				(void)CreateSlot(queueType, frameIndex);
			}
		}
	}
}

VulkanCommandRecordingContext::CommandSlot&
VulkanCommandRecordingContext::CreateSlot(
    ERhiQueueType queueType,
    std::uint32_t frameIndex)
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	auto slot = std::make_unique<CommandSlot>();

	slot->Owner = this;
	slot->QueueType = queueType;
	slot->FrameSlot = frameIndex % static_cast<std::uint32_t>(m_frames.size());
	slot->ContextIndex =
	    static_cast<std::uint32_t>(frameState.Slots.size());

	CreateCommandPool(*slot);
	AllocateCommandBuffer(*slot);
	InitializeRecordingResources(*slot);
	NameSlotObjects(*slot);

	frameState.Slots.push_back(std::move(slot));
	return *frameState.Slots.back();
}

void VulkanCommandRecordingContext::CreateCommandPool(CommandSlot& slot)
{
	const VkCommandPoolCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
	    .queueFamilyIndex =
	        m_rhi->GetQueueFamilyIndex(slot.QueueType)};
	const VkResult result =
	    vkCreateCommandPool(
	        m_rhi->GetDevice(),
	        &createInfo,
	        nullptr,
	        &slot.CommandPool);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(
		    g_vulkanCommandRecordingLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateCommandPool", result));
	}
}

void VulkanCommandRecordingContext::AllocateCommandBuffer(CommandSlot& slot)
{
	const VkCommandBufferAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .commandPool = slot.CommandPool,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1};
	const VkResult result =
	    vkAllocateCommandBuffers(
	        m_rhi->GetDevice(),
	        &allocateInfo,
	        &slot.CommandBuffer);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(
		    g_vulkanCommandRecordingLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure(
		        "vkAllocateCommandBuffers",
		        result));
	}
}

void VulkanCommandRecordingContext::InitializeRecordingResources(
    CommandSlot& slot)
{
	slot.DescriptorPool =
	    std::make_unique<VulkanRecordingDescriptorPool>(*m_rhi);
	slot.UploadPage =
	    std::make_unique<VulkanRecordingUploadPage>(
	        *m_rhi,
	        *m_memoryAllocator,
	        UploadPageCapacityInBytes);

	slot.CommandList = std::make_unique<VulkanRenderCommandList>();
	slot.CommandList->SetRhi(m_rhi);
	slot.CommandList->SetMemoryAllocator(m_memoryAllocator);
	slot.CommandList->SetDescriptorService(m_descriptorService);
	slot.CommandList->SetDescriptorAllocator(&m_descriptorService->m_allocator);
	slot.CommandList->SetRecordingDescriptorPool(slot.DescriptorPool.get());
	slot.CommandList->SetRecordingUploadPage(slot.UploadPage.get());
	slot.CommandList->SetQueueType(slot.QueueType);
	slot.CommandList->SetNativeCommandBuffer(
	    slot.CommandBuffer,
	    m_rhi->GetCmdBeginDebugUtilsLabel(),
	    m_rhi->GetCmdEndDebugUtilsLabel(),
	    m_rhi->GetCmdInsertDebugUtilsLabel());
}

void VulkanCommandRecordingContext::NameSlotObjects(
    const CommandSlot& slot) const noexcept
{
	PFN_vkSetDebugUtilsObjectNameEXT setObjectName =
	    m_rhi->GetSetDebugUtilsObjectName();
	if (setObjectName == nullptr)
	{
		return;
	}

	const std::string poolName = std::format(
	    "Sparkle Vulkan {} Command Pool Frame {} Context {}",
	    RhiQueueTypeToString(slot.QueueType),
	    slot.FrameSlot,
	    slot.ContextIndex);
	(void)VulkanDebugNames::SetObjectName(
	    setObjectName,
	    m_rhi->GetDevice(),
	    VK_OBJECT_TYPE_COMMAND_POOL,
	    reinterpret_cast<std::uint64_t>(slot.CommandPool),
	    poolName);

	const std::string commandBufferName = std::format(
	    "Sparkle Vulkan {} Command Buffer Frame {} Context {}",
	    RhiQueueTypeToString(slot.QueueType),
	    slot.FrameSlot,
	    slot.ContextIndex);
	(void)VulkanDebugNames::SetObjectName(
	    setObjectName,
	    m_rhi->GetDevice(),
	    VK_OBJECT_TYPE_COMMAND_BUFFER,
	    reinterpret_cast<std::uint64_t>(slot.CommandBuffer),
	    commandBufferName);

	const std::string descriptorPoolName = std::format(
	    "Sparkle Vulkan {} Recording Descriptor Pool Frame {} Context {}",
	    RhiQueueTypeToString(slot.QueueType),
	    slot.FrameSlot,
	    slot.ContextIndex);
	(void)VulkanDebugNames::SetObjectName(
	    setObjectName,
	    m_rhi->GetDevice(),
	    VK_OBJECT_TYPE_DESCRIPTOR_POOL,
	    reinterpret_cast<std::uint64_t>(
	        slot.DescriptorPool->GetNativePool()),
	    descriptorPoolName);
}

void VulkanCommandRecordingContext::WaitForFrameStateRetirement(
    const QueueFrameState& frameState) noexcept
{
	RhiSubmissionState retirement;
	for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
	{
		retirement.MarkUsed(slot->RetirementToken);
	}

	std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens{};
	const std::size_t tokenCount = retirement.CopyTokens(tokens);
	for (std::size_t tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex)
	{
		const RhiSubmissionToken token = tokens[tokenIndex];
		m_rhi->GetCommandQueue(token.Queue).WaitForSubmission(token.Value);
	}
}

void VulkanCommandRecordingContext::ResetSlot(CommandSlot& slot) noexcept
{
	assert(
	    slot.State == SlotState::Submitted ||
	    slot.State == SlotState::Discarded);
	assert(
	    !slot.RetirementToken.IsValid() ||
	    m_rhi->GetCommandQueue(slot.RetirementToken.Queue)
	        .IsSubmissionComplete(slot.RetirementToken.Value));

	slot.DescriptorPool->Reset();
	slot.UploadPage->Reset();
	slot.CommandList->AbandonTransientAllocationUses();
	slot.CommandList->ResetTrackedResources();
	slot.CommandList->ResetBoundState();
	slot.CommandList->SetRecording(false);
	slot.CommandList->SetRecordingOwner({});
	slot.RecordingOwner = {};
	slot.RecordingThread = {};
	slot.State = SlotState::Available;
	slot.RequiresPoolReset = true;
}

void VulkanCommandRecordingContext::ResetCommandPool(
    CommandSlot& slot) noexcept
{
	if (!slot.RequiresPoolReset)
	{
		return;
	}

	const VkResult result =
	    vkResetCommandPool(m_rhi->GetDevice(), slot.CommandPool, 0);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(
		    g_vulkanCommandRecordingLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkResetCommandPool", result));
	}

	slot.RequiresPoolReset = false;
}

void VulkanCommandRecordingContext::BeginSlot(CommandSlot& slot) noexcept
{
	if (slot.State != SlotState::Recording)
	{
		FailOwnershipViolation(slot, "begin a command buffer outside its recording lease");
	}

	const std::thread::id thread = std::this_thread::get_id();
	if (slot.RecordingThread == thread)
	{
		return;
	}
	if (slot.RecordingThread != std::thread::id{})
	{
		FailOwnershipViolation(slot, "record a command buffer from a second thread");
	}

	ResetCommandPool(slot);

	const VkCommandBufferBeginInfo beginInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = nullptr};
	const VkResult result =
	    vkBeginCommandBuffer(slot.CommandBuffer, &beginInfo);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(
		    g_vulkanCommandRecordingLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkBeginCommandBuffer", result));
	}

	slot.RecordingThread = thread;
	slot.CommandList->SetRecording(true);
}

void VulkanCommandRecordingContext::CloseSlot(CommandSlot& slot) noexcept
{
	if (slot.State != SlotState::Recording)
	{
		FailOwnershipViolation(slot, "close a command buffer outside its recording lease");
	}

	BeginSlot(slot);
	if (slot.RecordingThread != std::this_thread::get_id())
	{
		FailOwnershipViolation(slot, "close a command buffer from a second thread");
	}

	slot.CommandList->CloseOpenRendering();
	const VkResult result = vkEndCommandBuffer(slot.CommandBuffer);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(
		    g_vulkanCommandRecordingLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkEndCommandBuffer", result));
	}

	slot.CommandList->SetRecording(false);
	slot.State = SlotState::Closed;
}

void VulkanCommandRecordingContext::ReleaseSlot(CommandSlot& slot) noexcept
{
	if (slot.State == SlotState::Recording)
	{
		if (slot.RecordingThread != std::thread::id{} &&
		    slot.RecordingThread != std::this_thread::get_id())
		{
			FailOwnershipViolation(
			    slot,
			    "release a recording lease from a second thread");
		}

		slot.CommandList->SetRecording(false);
		slot.State = SlotState::Discarded;
	}
	else if (slot.State == SlotState::Closed)
	{
		slot.State = SlotState::Discarded;
	}
	else if (slot.State != SlotState::Available)
	{
		FailOwnershipViolation(
		    slot,
		    "release a recording lease after submission");
	}
}

VulkanCommandRecordingContext::CommandSlot*
VulkanCommandRecordingContext::ConsumeClosedLease(
    RhiCommandRecordingLease&& lease) noexcept
{
	return RhiCommandRecordingLeaseAccess::ConsumeClosed<CommandSlot>(
	    std::move(lease),
	    this,
	    SlotState::Closed);
}

void VulkanCommandRecordingContext::ResolveSubmittedSlot(
    CommandSlot& slot,
    RhiSubmissionToken token) noexcept
{
	slot.CommandList->ResolveTrackedResources(token);
	slot.CommandList->ResolveTransientAllocationUses(token);
	slot.RetirementToken = token;
	slot.State = SlotState::Submitted;
}

void VulkanCommandRecordingContext::DestroySlots() noexcept
{
	m_owner.AssertAccess();
	for (auto& frame : m_frames)
	{
		for (QueueFrameState& queue : frame)
		{
			queue.CurrentLease.reset();
			for (const std::unique_ptr<CommandSlot>& slot : queue.Slots)
			{
				if (slot->RetirementToken.IsValid())
				{
					m_rhi->GetCommandQueue(slot->RetirementToken.Queue)
					    .WaitForSubmission(slot->RetirementToken.Value);
				}

				slot->CommandList.reset();
				slot->UploadPage.reset();
				slot->DescriptorPool.reset();

				if (slot->CommandPool != VK_NULL_HANDLE)
				{
					vkDestroyCommandPool(
					    m_rhi->GetDevice(),
					    slot->CommandPool,
					    nullptr);
					slot->CommandPool = VK_NULL_HANDLE;
					slot->CommandBuffer = VK_NULL_HANDLE;
				}
			}
			queue.Slots.clear();
		}
	}
}

void VulkanCommandRecordingContext::BeginLease(void* state) noexcept
{
	auto& slot = *static_cast<CommandSlot*>(state);
	slot.Owner->BeginSlot(slot);
}

void VulkanCommandRecordingContext::CloseLease(void* state) noexcept
{
	auto& slot = *static_cast<CommandSlot*>(state);
	slot.Owner->CloseSlot(slot);
}

void VulkanCommandRecordingContext::ReleaseLease(
    void* state,
    bool) noexcept
{
	auto& slot = *static_cast<CommandSlot*>(state);
	slot.Owner->ReleaseSlot(slot);
}

[[noreturn]] void VulkanCommandRecordingContext::FailExhausted(
    ERhiQueueType queueType,
    std::uint32_t frameIndex) noexcept
{
	SPDLOG_LOGGER_CRITICAL(
	    g_vulkanCommandRecordingLogger,
	    "Vulkan {} recording contexts exhausted for frame slot {}.",
	    RhiQueueTypeToString(queueType),
	    frameIndex % static_cast<std::uint32_t>(m_frames.size()));
	std::terminate();
}

[[noreturn]] void VulkanCommandRecordingContext::FailOwnershipViolation(
    const CommandSlot& slot,
    const char* operation) noexcept
{
	SPDLOG_LOGGER_CRITICAL(
	    g_vulkanCommandRecordingLogger,
	    "Vulkan {} command context {} for frame slot {} cannot {}.",
	    RhiQueueTypeToString(slot.QueueType),
	    slot.ContextIndex,
	    slot.FrameSlot,
	    operation != nullptr ? operation : "perform the requested operation");
	std::terminate();
}
