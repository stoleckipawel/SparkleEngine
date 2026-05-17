#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanCommandContext.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"

#include <algorithm>
#include <format>

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
	WaitForIdle();
	for (FrameState& frameState : m_frames)
	{
		DestroyFrameState(frameState);
	}
}

void VulkanCommandContext::BeginFrame(std::uint32_t frameIndex) noexcept
{
	FrameState& frameState = GetFrameState(frameIndex);
	if (frameState.Fence != VK_NULL_HANDLE)
	{
		(void)vkWaitForFences(m_rhi.GetDevice(), 1, &frameState.Fence, VK_TRUE, UINT64_MAX);
		m_completedRetireFenceValue = std::max(m_completedRetireFenceValue, frameState.RetireFenceValue);
		(void)vkResetFences(m_rhi.GetDevice(), 1, &frameState.Fence);
	}

	if (frameState.CommandPool != VK_NULL_HANDLE)
	{
		const VkResult resetPoolResult = vkResetCommandPool(m_rhi.GetDevice(), frameState.CommandPool, 0);
		if (!VulkanResult::Succeeded(resetPoolResult))
		{
			Diagnostics::Fail(g_vulkanCommandContextLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkResetCommandPool", resetPoolResult));
		}
	}

	const VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .pNext = nullptr, .flags = 0, .pInheritanceInfo = nullptr};
	const VkResult beginResult = vkBeginCommandBuffer(frameState.CommandBuffer, &beginInfo);
	if (!VulkanResult::Succeeded(beginResult))
	{
		Diagnostics::Fail(g_vulkanCommandContextLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkBeginCommandBuffer", beginResult));
	}
	frameState.IsRecording = true;
}

void VulkanCommandContext::SubmitFrame(std::uint32_t frameIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) noexcept
{
	FrameState& frameState = GetFrameState(frameIndex);
	if (frameState.IsRecording)
	{
		const VkResult endResult = vkEndCommandBuffer(frameState.CommandBuffer);
		if (!VulkanResult::Succeeded(endResult))
		{
			Diagnostics::Fail(g_vulkanCommandContextLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkEndCommandBuffer", endResult));
		}
		frameState.IsRecording = false;
	}

	const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	const VkSubmitInfo submitInfo{
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .pNext = nullptr,
	    .waitSemaphoreCount = waitSemaphore != VK_NULL_HANDLE ? 1u : 0u,
	    .pWaitSemaphores = waitSemaphore != VK_NULL_HANDLE ? &waitSemaphore : nullptr,
	    .pWaitDstStageMask = waitSemaphore != VK_NULL_HANDLE ? &waitStage : nullptr,
	    .commandBufferCount = 1,
	    .pCommandBuffers = &frameState.CommandBuffer,
	    .signalSemaphoreCount = signalSemaphore != VK_NULL_HANDLE ? 1u : 0u,
	    .pSignalSemaphores = signalSemaphore != VK_NULL_HANDLE ? &signalSemaphore : nullptr};

	frameState.RetireFenceValue = m_nextRetireFenceValue++;
	const VkResult submitResult = vkQueueSubmit(m_rhi.GetGraphicsQueue(), 1, &submitInfo, frameState.Fence);
	if (!VulkanResult::Succeeded(submitResult))
	{
		Diagnostics::Fail(g_vulkanCommandContextLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkQueueSubmit", submitResult));
	}
}

void VulkanCommandContext::CancelFrame(std::uint32_t frameIndex) noexcept
{
	FrameState& frameState = GetFrameState(frameIndex);
	if (frameState.IsRecording)
	{
		(void)vkEndCommandBuffer(frameState.CommandBuffer);
		frameState.IsRecording = false;
	}
	if (frameState.Fence != VK_NULL_HANDLE)
	{
		const VkSubmitInfo submitInfo{
		    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		    .pNext = nullptr,
		    .waitSemaphoreCount = 0,
		    .pWaitSemaphores = nullptr,
		    .pWaitDstStageMask = nullptr,
		    .commandBufferCount = 1,
		    .pCommandBuffers = &frameState.CommandBuffer,
		    .signalSemaphoreCount = 0,
		    .pSignalSemaphores = nullptr};
		(void)vkResetFences(m_rhi.GetDevice(), 1, &frameState.Fence);
		frameState.RetireFenceValue = m_nextRetireFenceValue++;
		(void)vkQueueSubmit(m_rhi.GetGraphicsQueue(), 1, &submitInfo, frameState.Fence);
	}
}

void VulkanCommandContext::WaitForIdle() noexcept
{
	for (FrameState& frameState : m_frames)
	{
		if (frameState.Fence != VK_NULL_HANDLE)
		{
			(void)vkWaitForFences(m_rhi.GetDevice(), 1, &frameState.Fence, VK_TRUE, UINT64_MAX);
			m_completedRetireFenceValue = std::max(m_completedRetireFenceValue, frameState.RetireFenceValue);
		}
	}
}

VulkanRenderCommandList& VulkanCommandContext::GetCommandList(std::uint32_t frameIndex) noexcept
{
	return *GetFrameState(frameIndex).CommandList;
}

VkCommandBuffer VulkanCommandContext::GetCommandBuffer(std::uint32_t frameIndex) const noexcept
{
	return GetFrameState(frameIndex).CommandBuffer;
}

VkSemaphore VulkanCommandContext::GetImageAvailableSemaphore(std::uint32_t frameIndex) const noexcept
{
	return GetFrameState(frameIndex).ImageAvailableSemaphore;
}

VkSemaphore VulkanCommandContext::GetRenderFinishedSemaphore(std::uint32_t frameIndex) const noexcept
{
	return GetFrameState(frameIndex).RenderFinishedSemaphore;
}

VulkanCommandContext::FrameState& VulkanCommandContext::GetFrameState(std::uint32_t frameIndex) noexcept
{
	return m_frames[frameIndex % m_frames.size()];
}

const VulkanCommandContext::FrameState& VulkanCommandContext::GetFrameState(std::uint32_t frameIndex) const noexcept
{
	return m_frames[frameIndex % m_frames.size()];
}

void VulkanCommandContext::CreateFrameState(std::uint32_t frameIndex)
{
	FrameState& frameState = m_frames[frameIndex];
	const VkCommandPoolCreateInfo poolCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	    .queueFamilyIndex = m_rhi.GetGraphicsQueueFamilyIndex()};
	VkResult result = vkCreateCommandPool(m_rhi.GetDevice(), &poolCreateInfo, nullptr, &frameState.CommandPool);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(g_vulkanCommandContextLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateCommandPool", result));
	}

	const VkCommandBufferAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .commandPool = frameState.CommandPool,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1};
	result = vkAllocateCommandBuffers(m_rhi.GetDevice(), &allocateInfo, &frameState.CommandBuffer);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(g_vulkanCommandContextLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkAllocateCommandBuffers", result));
	}

	const VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT};
	result = vkCreateFence(m_rhi.GetDevice(), &fenceInfo, nullptr, &frameState.Fence);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(g_vulkanCommandContextLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateFence", result));
	}

	const VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0};
	result = vkCreateSemaphore(m_rhi.GetDevice(), &semaphoreInfo, nullptr, &frameState.ImageAvailableSemaphore);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(g_vulkanCommandContextLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateSemaphore", result));
	}
	result = vkCreateSemaphore(m_rhi.GetDevice(), &semaphoreInfo, nullptr, &frameState.RenderFinishedSemaphore);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(g_vulkanCommandContextLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateSemaphore", result));
	}

	frameState.CommandList = std::make_unique<VulkanRenderCommandList>();
	frameState.CommandList->SetNativeCommandBuffer(
	    frameState.CommandBuffer,
	    m_rhi.GetCmdBeginDebugUtilsLabel(),
	    m_rhi.GetCmdEndDebugUtilsLabel(),
	    m_rhi.GetCmdInsertDebugUtilsLabel());
	NameFrameState(frameIndex, frameState);
}

void VulkanCommandContext::DestroyFrameState(FrameState& frameState) noexcept
{
	frameState.CommandList.reset();
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
	if (frameState.Fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(m_rhi.GetDevice(), frameState.Fence, nullptr);
		frameState.Fence = VK_NULL_HANDLE;
	}
	if (frameState.CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_rhi.GetDevice(), frameState.CommandPool, nullptr);
		frameState.CommandPool = VK_NULL_HANDLE;
		frameState.CommandBuffer = VK_NULL_HANDLE;
	}
}

void VulkanCommandContext::NameFrameState(std::uint32_t frameIndex, FrameState& frameState) noexcept
{
	PFN_vkSetDebugUtilsObjectNameEXT setObjectName = m_rhi.GetSetDebugUtilsObjectName();
	if (setObjectName == nullptr)
	{
		return;
	}

	const std::string poolName = std::format("Sparkle Vulkan Graphics Command Pool Frame {}", frameIndex);
	const std::string commandBufferName = std::format("Sparkle Vulkan Graphics Command Buffer Frame {}", frameIndex);
	const std::string fenceName = std::format("Sparkle Vulkan Frame Fence {}", frameIndex);
	const std::string imageAvailableName = std::format("Sparkle Vulkan Image Available Semaphore {}", frameIndex);
	const std::string renderFinishedName = std::format("Sparkle Vulkan Render Finished Semaphore {}", frameIndex);
	(void)VulkanDebugNames::SetObjectName(setObjectName, m_rhi.GetDevice(), VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<std::uint64_t>(frameState.CommandPool), poolName);
	(void)VulkanDebugNames::SetObjectName(setObjectName, m_rhi.GetDevice(), VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<std::uint64_t>(frameState.CommandBuffer), commandBufferName);
	(void)VulkanDebugNames::SetObjectName(setObjectName, m_rhi.GetDevice(), VK_OBJECT_TYPE_FENCE, reinterpret_cast<std::uint64_t>(frameState.Fence), fenceName);
	(void)VulkanDebugNames::SetObjectName(
	    setObjectName,
	    m_rhi.GetDevice(),
	    VK_OBJECT_TYPE_SEMAPHORE,
	    reinterpret_cast<std::uint64_t>(frameState.ImageAvailableSemaphore),
	    imageAvailableName);
	(void)VulkanDebugNames::SetObjectName(
	    setObjectName,
	    m_rhi.GetDevice(),
	    VK_OBJECT_TYPE_SEMAPHORE,
	    reinterpret_cast<std::uint64_t>(frameState.RenderFinishedSemaphore),
	    renderFinishedName);
}