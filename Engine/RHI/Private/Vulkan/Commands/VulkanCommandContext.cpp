#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanCommandContext.h"

#include "Vulkan/Commands/VulkanCommandQueue.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Descriptors/VulkanDescriptorManager.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"

#include <format>
#include <string>

static const auto g_vulkanCommandContextLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Commands");

VulkanCommandContext::VulkanCommandContext(VulkanRhi& rhi) : m_rhi(rhi)
{
	for (std::uint32_t frameIndex = 0; frameIndex < m_frames.size(); ++frameIndex)
	{
		CreateFrameState(frameIndex);
	}
}

VulkanCommandContext::~VulkanCommandContext() noexcept
{
	m_rhi.WaitForIdle();
	for (FrameState& frameState : m_frames)
	{
		DestroyFrameState(frameState);
	}
}

void VulkanCommandContext::BeginFrame(std::uint32_t frameIndex) noexcept
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		QueueFrameState& frameState = GetQueueFrameState(static_cast<ERhiQueueType>(queueIndex), frameIndex);
		if (frameState.LastSubmission.IsValid())
		{
			m_rhi.GetCommandQueue(frameState.LastSubmission.Queue).WaitForSubmission(frameState.LastSubmission.Value);
		}
		if (frameState.CommandPool != VK_NULL_HANDLE)
		{
			const VkResult resetPoolResult = vkResetCommandPool(m_rhi.GetDevice(), frameState.CommandPool, 0);
			if (!VulkanResult::Succeeded(resetPoolResult))
			{
				Diagnostics::Fail(
				    g_vulkanCommandContextLogger,
				    __FILE__,
				    __LINE__,
				    VulkanResult::FormatFailure("vkResetCommandPool", resetPoolResult));
			}
		}
		frameState.NextSlot = 0;
		frameState.CurrentSlot = nullptr;
		for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
		{
			slot->IsRecording = false;
		}
	}
}

VulkanRenderCommandList& VulkanCommandContext::BeginCommandList(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	CommandSlot& slot = GetOrCreateSlot(queueType, frameIndex);

	const VkCommandBufferBeginInfo beginInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = nullptr};
	const VkResult beginResult = vkBeginCommandBuffer(slot.CommandBuffer, &beginInfo);
	if (!VulkanResult::Succeeded(beginResult))
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkBeginCommandBuffer", beginResult));
	}
	slot.CommandList->ResetTrackedResources();
	slot.CommandList->ResetBoundState();
	slot.IsRecording = true;
	frameState.CurrentSlot = &slot;
	return *slot.CommandList;
}

RhiSubmissionToken VulkanCommandContext::SubmitCommandList(
	VulkanRenderCommandList& commandList,
	std::uint32_t frameIndex,
	std::span<const RhiSubmissionToken> waitTokens,
	VkSemaphore binaryWaitSemaphore,
	VkPipelineStageFlags binaryWaitStage,
	VkSemaphore binarySignalSemaphore) noexcept
{
	const ERhiQueueType queueType = commandList.GetQueueType();
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	CommandSlot* slot = FindSlot(commandList, frameIndex);
	if (slot == nullptr || !slot->IsRecording)
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    "SubmitCommandList requires a recording command list from the current frame");
		return {};
	}

	commandList.CloseOpenRendering();
	const VkResult endResult = vkEndCommandBuffer(slot->CommandBuffer);
	if (!VulkanResult::Succeeded(endResult))
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkEndCommandBuffer", endResult));
	}
	slot->IsRecording = false;

	frameState.LastSubmission = m_rhi.GetCommandQueue(queueType).Submit(
	    VulkanQueueSubmission{
	        .CommandBuffer = slot->CommandBuffer,
	        .WaitTokens = waitTokens,
	        .BinaryWaitSemaphore = binaryWaitSemaphore,
	        .BinaryWaitStage = binaryWaitStage,
	        .BinarySignalSemaphore = binarySignalSemaphore});
	return frameState.LastSubmission;
}

void VulkanCommandContext::CancelFrame(std::uint32_t frameIndex) noexcept
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		QueueFrameState& frameState = GetQueueFrameState(static_cast<ERhiQueueType>(queueIndex), frameIndex);
		for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
		{
			if (slot->IsRecording)
			{
				(void)SubmitCommandList(*slot->CommandList, frameIndex);
			}
		}
	}
}

void VulkanCommandContext::ConfigureCommandLists(
	VulkanGpuMemoryAllocator& memoryAllocator,
	VulkanDescriptorManager& descriptorManager,
	VulkanDescriptorAllocator& descriptorAllocator) noexcept
{
	m_memoryAllocator = &memoryAllocator;
	m_descriptorManager = &descriptorManager;
	m_descriptorAllocator = &descriptorAllocator;
	for (FrameState& frame : m_frames)
	{
		for (QueueFrameState& queueFrame : frame.Queues)
		{
			for (const std::unique_ptr<CommandSlot>& slot : queueFrame.Slots)
			{
				ConfigureCommandList(*slot->CommandList);
			}
		}
	}
}

VulkanRenderCommandList& VulkanCommandContext::GetCommandList(std::uint32_t frameIndex) noexcept
{
	return GetCommandList(ERhiQueueType::Graphics, frameIndex);
}

VulkanRenderCommandList& VulkanCommandContext::GetCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	if (frameState.CurrentSlot == nullptr)
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    "GetCommandList called before BeginCommandList");
	}
	return *frameState.CurrentSlot->CommandList;
}

VkCommandBuffer VulkanCommandContext::GetCommandBuffer(std::uint32_t frameIndex) const noexcept
{
	return GetCommandBuffer(ERhiQueueType::Graphics, frameIndex);
}

VkCommandBuffer VulkanCommandContext::GetCommandBuffer(ERhiQueueType queueType, std::uint32_t frameIndex) const noexcept
{
	const QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	return frameState.CurrentSlot != nullptr ? frameState.CurrentSlot->CommandBuffer : VK_NULL_HANDLE;
}

bool VulkanCommandContext::IsCommandBufferRecording(VkCommandBuffer commandBuffer) const noexcept
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return false;
	}
	for (const FrameState& frame : m_frames)
	{
		for (const QueueFrameState& queueFrame : frame.Queues)
		{
			for (const std::unique_ptr<CommandSlot>& slot : queueFrame.Slots)
			{
				if (slot->CommandBuffer == commandBuffer)
				{
					return slot->IsRecording;
				}
			}
		}
	}
	return false;
}

bool VulkanCommandContext::IsRecording(
	const VulkanRenderCommandList& commandList,
	std::uint32_t frameIndex) const noexcept
{
	const CommandSlot* slot = FindSlot(commandList, frameIndex);
	return slot != nullptr && slot->IsRecording;
}

VulkanCommandContext::QueueFrameState& VulkanCommandContext::GetQueueFrameState(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) noexcept
{
	return m_frames[frameIndex % m_frames.size()].Queues[RhiQueueTypeToIndex(queueType)];
}

const VulkanCommandContext::QueueFrameState& VulkanCommandContext::GetQueueFrameState(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) const noexcept
{
	return m_frames[frameIndex % m_frames.size()].Queues[RhiQueueTypeToIndex(queueType)];
}

void VulkanCommandContext::CreateFrameState(std::uint32_t frameIndex)
{
	FrameState& frame = m_frames[frameIndex];
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		const ERhiQueueType queueType = static_cast<ERhiQueueType>(queueIndex);
		QueueFrameState& queueFrame = frame.Queues[queueIndex];
		const VkCommandPoolCreateInfo poolCreateInfo{
		    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		    .pNext = nullptr,
		    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		    .queueFamilyIndex = m_rhi.GetQueueFamilyIndex(queueType)};
		const VkResult result = vkCreateCommandPool(m_rhi.GetDevice(), &poolCreateInfo, nullptr, &queueFrame.CommandPool);
		if (!VulkanResult::Succeeded(result))
		{
			Diagnostics::Fail(
			    g_vulkanCommandContextLogger,
			    __FILE__,
			    __LINE__,
			    VulkanResult::FormatFailure("vkCreateCommandPool", result));
		}
		NameQueueFrameState(queueType, frameIndex, queueFrame);
	}
}

void VulkanCommandContext::DestroyFrameState(FrameState& frameState) noexcept
{
	for (QueueFrameState& queueFrame : frameState.Queues)
	{
		queueFrame.Slots.clear();
		if (queueFrame.CommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_rhi.GetDevice(), queueFrame.CommandPool, nullptr);
			queueFrame.CommandPool = VK_NULL_HANDLE;
		}
	}
}

void VulkanCommandContext::NameQueueFrameState(
	ERhiQueueType queueType,
	std::uint32_t frameIndex,
	QueueFrameState& frameState) noexcept
{
	PFN_vkSetDebugUtilsObjectNameEXT setObjectName = m_rhi.GetSetDebugUtilsObjectName();
	if (setObjectName == nullptr)
	{
		return;
	}

	const std::string poolName = std::format("Sparkle Vulkan {} Command Pool Frame {}", RhiQueueTypeToString(queueType), frameIndex);
	(void)VulkanDebugNames::SetObjectName(
	    setObjectName,
	    m_rhi.GetDevice(),
	    VK_OBJECT_TYPE_COMMAND_POOL,
	    reinterpret_cast<std::uint64_t>(frameState.CommandPool),
	    poolName);
}

VulkanCommandContext::CommandSlot& VulkanCommandContext::GetOrCreateSlot(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	const std::size_t slotIndex = frameState.NextSlot++;
	if (slotIndex < frameState.Slots.size())
	{
		return *frameState.Slots[slotIndex];
	}

	auto slot = std::make_unique<CommandSlot>();
	const VkCommandBufferAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .commandPool = frameState.CommandPool,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1};
	const VkResult result = vkAllocateCommandBuffers(m_rhi.GetDevice(), &allocateInfo, &slot->CommandBuffer);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkAllocateCommandBuffers", result));
	}

	slot->CommandList = std::make_unique<VulkanRenderCommandList>();
	slot->CommandList->SetQueueType(queueType);
	slot->CommandList->SetNativeCommandBuffer(
	    slot->CommandBuffer,
	    m_rhi.GetCmdBeginDebugUtilsLabel(),
	    m_rhi.GetCmdEndDebugUtilsLabel(),
	    m_rhi.GetCmdInsertDebugUtilsLabel());
	ConfigureCommandList(*slot->CommandList);
	NameCommandSlot(queueType, frameIndex, slotIndex, *slot);
	frameState.Slots.push_back(std::move(slot));
	return *frameState.Slots.back();
}

VulkanCommandContext::CommandSlot* VulkanCommandContext::FindSlot(
	VulkanRenderCommandList& commandList,
	std::uint32_t frameIndex) noexcept
{
	return const_cast<CommandSlot*>(std::as_const(*this).FindSlot(commandList, frameIndex));
}

const VulkanCommandContext::CommandSlot* VulkanCommandContext::FindSlot(
	const VulkanRenderCommandList& commandList,
	std::uint32_t frameIndex) const noexcept
{
	const QueueFrameState& frameState = GetQueueFrameState(commandList.GetQueueType(), frameIndex);
	for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
	{
		if (slot->CommandList.get() == &commandList)
		{
			return slot.get();
		}
	}
	return nullptr;
}

void VulkanCommandContext::ConfigureCommandList(VulkanRenderCommandList& commandList) noexcept
{
	commandList.SetRhi(&m_rhi);
	commandList.SetMemoryAllocator(m_memoryAllocator);
	commandList.SetDescriptorManager(m_descriptorManager);
	commandList.SetDescriptorAllocator(m_descriptorAllocator);
}

void VulkanCommandContext::NameCommandSlot(
	ERhiQueueType queueType,
	std::uint32_t frameIndex,
	std::size_t slotIndex,
	CommandSlot& slot) noexcept
{
	PFN_vkSetDebugUtilsObjectNameEXT setObjectName = m_rhi.GetSetDebugUtilsObjectName();
	if (setObjectName == nullptr)
	{
		return;
	}
	const std::string name = std::format(
	    "Sparkle Vulkan {} Command Buffer Frame {} Slot {}",
	    RhiQueueTypeToString(queueType),
	    frameIndex,
	    slotIndex);
	(void)VulkanDebugNames::SetObjectName(
	    setObjectName,
	    m_rhi.GetDevice(),
	    VK_OBJECT_TYPE_COMMAND_BUFFER,
	    reinterpret_cast<std::uint64_t>(slot.CommandBuffer),
	    name);
}
