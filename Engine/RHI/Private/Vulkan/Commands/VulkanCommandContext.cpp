#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanCommandContext.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"

#include <array>
#include <format>
#include <string>
#include <vector>

static const auto g_vulkanCommandContextLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Commands");

VulkanCommandContext::VulkanCommandContext(VulkanRhi& rhi) : m_rhi(rhi)
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		CreateQueueState(static_cast<ERhiQueueType>(queueIndex));
	}
	for (std::uint32_t frameIndex = 0; frameIndex < m_frames.size(); ++frameIndex)
	{
		CreateFrameState(frameIndex);
	}
}

VulkanCommandContext::~VulkanCommandContext() noexcept
{
	WaitForIdle();
	for (FrameState& frameState : m_frames)
	{
		DestroyFrameState(frameState);
	}
	for (QueueState& queueState : m_queues)
	{
		DestroyQueueState(queueState);
	}
}

void VulkanCommandContext::BeginFrame(std::uint32_t frameIndex) noexcept
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		WaitForSubmission(
		    GetQueueFrameState(static_cast<ERhiQueueType>(queueIndex), frameIndex).SubmissionToken);
	}
	BeginCommandList(ERhiQueueType::Graphics, frameIndex);
}

void VulkanCommandContext::BeginCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	WaitForSubmission(frameState.SubmissionToken);

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

	const VkCommandBufferBeginInfo beginInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = nullptr};
	const VkResult beginResult = vkBeginCommandBuffer(frameState.CommandBuffer, &beginInfo);
	if (!VulkanResult::Succeeded(beginResult))
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkBeginCommandBuffer", beginResult));
	}
	frameState.IsRecording = true;
}

RhiSubmissionToken VulkanCommandContext::SubmitFrame(
	std::uint32_t frameIndex,
	VkSemaphore waitSemaphore,
	VkSemaphore signalSemaphore) noexcept
{
	return SubmitCommandList(
	    ERhiQueueType::Graphics,
	    frameIndex,
	    {},
	    waitSemaphore,
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    signalSemaphore);
}

RhiSubmissionToken VulkanCommandContext::SubmitCommandList(
	ERhiQueueType queueType,
	std::uint32_t frameIndex,
	std::span<const RhiSubmissionToken> waitTokens,
	VkSemaphore binaryWaitSemaphore,
	VkPipelineStageFlags binaryWaitStage,
	VkSemaphore binarySignalSemaphore) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	if (frameState.IsRecording)
	{
		if (frameState.CommandList)
		{
			frameState.CommandList->CloseOpenRendering();
		}
		const VkResult endResult = vkEndCommandBuffer(frameState.CommandBuffer);
		if (!VulkanResult::Succeeded(endResult))
		{
			Diagnostics::Fail(
			    g_vulkanCommandContextLogger,
			    __FILE__,
			    __LINE__,
			    VulkanResult::FormatFailure("vkEndCommandBuffer", endResult));
		}
		frameState.IsRecording = false;
	}

	std::vector<VkSemaphore> waitSemaphores;
	std::vector<std::uint64_t> waitValues;
	std::vector<VkPipelineStageFlags> waitStages;
	waitSemaphores.reserve(waitTokens.size() + (binaryWaitSemaphore != VK_NULL_HANDLE ? 1u : 0u));
	waitValues.reserve(waitSemaphores.capacity());
	waitStages.reserve(waitSemaphores.capacity());
	for (const RhiSubmissionToken token : waitTokens)
	{
		if (!token.IsValid() || token.Queue == queueType)
		{
			continue;
		}
		waitSemaphores.push_back(m_queues[RhiQueueTypeToIndex(token.Queue)].TimelineSemaphore);
		waitValues.push_back(token.Value);
		waitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}
	if (binaryWaitSemaphore != VK_NULL_HANDLE)
	{
		waitSemaphores.push_back(binaryWaitSemaphore);
		waitValues.push_back(0);
		waitStages.push_back(binaryWaitStage);
	}

	QueueState& queue = m_queues[RhiQueueTypeToIndex(queueType)];
	const std::uint64_t submissionValue = queue.NextSubmissionValue++;
	std::array<VkSemaphore, 2> signalSemaphores = {queue.TimelineSemaphore, binarySignalSemaphore};
	std::array<std::uint64_t, 2> signalValues = {submissionValue, 0};
	const std::uint32_t signalCount = binarySignalSemaphore != VK_NULL_HANDLE ? 2u : 1u;
	const VkTimelineSemaphoreSubmitInfo timelineInfo{
	    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
	    .pNext = nullptr,
	    .waitSemaphoreValueCount = static_cast<std::uint32_t>(waitValues.size()),
	    .pWaitSemaphoreValues = waitValues.empty() ? nullptr : waitValues.data(),
	    .signalSemaphoreValueCount = signalCount,
	    .pSignalSemaphoreValues = signalValues.data()};
	const VkSubmitInfo submitInfo{
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .pNext = &timelineInfo,
	    .waitSemaphoreCount = static_cast<std::uint32_t>(waitSemaphores.size()),
	    .pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data(),
	    .pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data(),
	    .commandBufferCount = 1,
	    .pCommandBuffers = &frameState.CommandBuffer,
	    .signalSemaphoreCount = signalCount,
	    .pSignalSemaphores = signalSemaphores.data()};

	const VkResult submitResult = vkQueueSubmit(m_rhi.GetQueue(queueType), 1, &submitInfo, VK_NULL_HANDLE);
	if (!VulkanResult::Succeeded(submitResult))
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkQueueSubmit", submitResult));
		return {};
	}

	queue.LastSubmittedValue = submissionValue;
	frameState.SubmissionToken = RhiSubmissionToken{.Queue = queueType, .Value = submissionValue};
	return frameState.SubmissionToken;
}

void VulkanCommandContext::CancelFrame(std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(ERhiQueueType::Graphics, frameIndex);
	if (frameState.IsRecording)
	{
		(void)SubmitCommandList(ERhiQueueType::Graphics, frameIndex);
	}
}

void VulkanCommandContext::WaitForIdle() noexcept
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		WaitForSubmission(GetLastSubmittedToken(static_cast<ERhiQueueType>(queueIndex)));
	}
}

void VulkanCommandContext::WaitForSubmission(RhiSubmissionToken token) noexcept
{
	if (!token.IsValid())
	{
		return;
	}

	const VkSemaphore semaphore = m_queues[RhiQueueTypeToIndex(token.Queue)].TimelineSemaphore;
	const VkSemaphoreWaitInfo waitInfo{
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .semaphoreCount = 1,
	    .pSemaphores = &semaphore,
	    .pValues = &token.Value};
	const VkResult waitResult = vkWaitSemaphores(m_rhi.GetDevice(), &waitInfo, UINT64_MAX);
	if (!VulkanResult::Succeeded(waitResult))
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkWaitSemaphores", waitResult));
	}
}

bool VulkanCommandContext::IsSubmissionComplete(RhiSubmissionToken token) const noexcept
{
	return !token.IsValid() || GetCompletedSubmissionValue(token.Queue) >= token.Value;
}

RhiSubmissionToken VulkanCommandContext::GetLastSubmittedToken(ERhiQueueType queueType) const noexcept
{
	return RhiSubmissionToken{
	    .Queue = queueType,
	    .Value = m_queues[RhiQueueTypeToIndex(queueType)].LastSubmittedValue};
}

std::uint64_t VulkanCommandContext::GetCompletedSubmissionValue(ERhiQueueType queueType) const noexcept
{
	std::uint64_t completedValue = 0;
	const VkResult result = vkGetSemaphoreCounterValue(
	    m_rhi.GetDevice(),
	    m_queues[RhiQueueTypeToIndex(queueType)].TimelineSemaphore,
	    &completedValue);
	return VulkanResult::Succeeded(result) ? completedValue : 0;
}

VulkanRenderCommandList& VulkanCommandContext::GetCommandList(std::uint32_t frameIndex) noexcept
{
	return GetCommandList(ERhiQueueType::Graphics, frameIndex);
}

VulkanRenderCommandList& VulkanCommandContext::GetCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept
{
	return *GetQueueFrameState(queueType, frameIndex).CommandList;
}

VkCommandBuffer VulkanCommandContext::GetCommandBuffer(std::uint32_t frameIndex) const noexcept
{
	return GetCommandBuffer(ERhiQueueType::Graphics, frameIndex);
}

VkCommandBuffer VulkanCommandContext::GetCommandBuffer(ERhiQueueType queueType, std::uint32_t frameIndex) const noexcept
{
	return GetQueueFrameState(queueType, frameIndex).CommandBuffer;
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
			if (queueFrame.CommandBuffer == commandBuffer)
			{
				return queueFrame.IsRecording;
			}
		}
	}
	return false;
}

VkSemaphore VulkanCommandContext::GetImageAvailableSemaphore(std::uint32_t frameIndex) const noexcept
{
	return m_frames[frameIndex % m_frames.size()].ImageAvailableSemaphore;
}

VkSemaphore VulkanCommandContext::GetRenderFinishedSemaphore(std::uint32_t frameIndex) const noexcept
{
	return m_frames[frameIndex % m_frames.size()].RenderFinishedSemaphore;
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

void VulkanCommandContext::CreateQueueState(ERhiQueueType queueType)
{
	QueueState& queue = m_queues[RhiQueueTypeToIndex(queueType)];
	const VkSemaphoreTypeCreateInfo timelineCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
	    .pNext = nullptr,
	    .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
	    .initialValue = 0};
	const VkSemaphoreCreateInfo semaphoreInfo{
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	    .pNext = &timelineCreateInfo,
	    .flags = 0};
	const VkResult result = vkCreateSemaphore(m_rhi.GetDevice(), &semaphoreInfo, nullptr, &queue.TimelineSemaphore);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateSemaphore(timeline)", result));
	}
}

void VulkanCommandContext::DestroyQueueState(QueueState& queueState) noexcept
{
	if (queueState.TimelineSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(m_rhi.GetDevice(), queueState.TimelineSemaphore, nullptr);
		queueState.TimelineSemaphore = VK_NULL_HANDLE;
	}
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
		VkResult result = vkCreateCommandPool(m_rhi.GetDevice(), &poolCreateInfo, nullptr, &queueFrame.CommandPool);
		if (!VulkanResult::Succeeded(result))
		{
			Diagnostics::Fail(
			    g_vulkanCommandContextLogger,
			    __FILE__,
			    __LINE__,
			    VulkanResult::FormatFailure("vkCreateCommandPool", result));
		}

		const VkCommandBufferAllocateInfo allocateInfo{
		    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		    .pNext = nullptr,
		    .commandPool = queueFrame.CommandPool,
		    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		    .commandBufferCount = 1};
		result = vkAllocateCommandBuffers(m_rhi.GetDevice(), &allocateInfo, &queueFrame.CommandBuffer);
		if (!VulkanResult::Succeeded(result))
		{
			Diagnostics::Fail(
			    g_vulkanCommandContextLogger,
			    __FILE__,
			    __LINE__,
			    VulkanResult::FormatFailure("vkAllocateCommandBuffers", result));
		}

		queueFrame.CommandList = std::make_unique<VulkanRenderCommandList>();
		queueFrame.CommandList->SetQueueType(queueType);
		queueFrame.CommandList->SetNativeCommandBuffer(
		    queueFrame.CommandBuffer,
		    m_rhi.GetCmdBeginDebugUtilsLabel(),
		    m_rhi.GetCmdEndDebugUtilsLabel(),
		    m_rhi.GetCmdInsertDebugUtilsLabel());
		NameQueueFrameState(queueType, frameIndex, queueFrame);
	}

	const VkSemaphoreCreateInfo semaphoreInfo{
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0};
	VkResult result = vkCreateSemaphore(m_rhi.GetDevice(), &semaphoreInfo, nullptr, &frame.ImageAvailableSemaphore);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateSemaphore(imageAvailable)", result));
	}
	result = vkCreateSemaphore(m_rhi.GetDevice(), &semaphoreInfo, nullptr, &frame.RenderFinishedSemaphore);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    g_vulkanCommandContextLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateSemaphore(renderFinished)", result));
	}
}

void VulkanCommandContext::DestroyFrameState(FrameState& frameState) noexcept
{
	for (QueueFrameState& queueFrame : frameState.Queues)
	{
		queueFrame.CommandList.reset();
		if (queueFrame.CommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_rhi.GetDevice(), queueFrame.CommandPool, nullptr);
			queueFrame.CommandPool = VK_NULL_HANDLE;
			queueFrame.CommandBuffer = VK_NULL_HANDLE;
		}
	}
	if (frameState.RenderFinishedSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(m_rhi.GetDevice(), frameState.RenderFinishedSemaphore, nullptr);
		frameState.RenderFinishedSemaphore = VK_NULL_HANDLE;
	}
	if (frameState.ImageAvailableSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(m_rhi.GetDevice(), frameState.ImageAvailableSemaphore, nullptr);
		frameState.ImageAvailableSemaphore = VK_NULL_HANDLE;
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
	const std::string commandBufferName =
	    std::format("Sparkle Vulkan {} Command Buffer Frame {}", RhiQueueTypeToString(queueType), frameIndex);
	(void)VulkanDebugNames::SetObjectName(
	    setObjectName,
	    m_rhi.GetDevice(),
	    VK_OBJECT_TYPE_COMMAND_POOL,
	    reinterpret_cast<std::uint64_t>(frameState.CommandPool),
	    poolName);
	(void)VulkanDebugNames::SetObjectName(
	    setObjectName,
	    m_rhi.GetDevice(),
	    VK_OBJECT_TYPE_COMMAND_BUFFER,
	    reinterpret_cast<std::uint64_t>(frameState.CommandBuffer),
	    commandBufferName);
}
