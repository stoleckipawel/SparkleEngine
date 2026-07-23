#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanCommandQueue.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"

#include <array>
#include <format>

class VulkanCommandQueuePolicy final
{
  public:
	static const std::shared_ptr<spdlog::logger>& Logger()
	{
		static const auto logger = Logging::GetOrCreateLogger("RHI.Vulkan.Queue");
		return logger;
	}

	#if SPARKLE_BUILD_SHIPPING
	static constexpr std::uint64_t GpuWaitTimeoutNanoseconds = UINT64_MAX;
	#else
	static constexpr std::uint64_t GpuWaitTimeoutNanoseconds = 30'000'000'000ull;
	#endif
};

VulkanCommandQueue::VulkanCommandQueue(
	VulkanRhi& rhi,
	ERhiQueueType queueType,
	std::shared_ptr<VulkanNativeQueueState> nativeQueue) noexcept :
	m_rhi(rhi), m_queueType(queueType), m_nativeQueue(std::move(nativeQueue))
{
	const VkSemaphoreTypeCreateInfo timelineCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
	    .pNext = nullptr,
	    .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
	    .initialValue = 0};
	const VkSemaphoreCreateInfo semaphoreInfo{
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	    .pNext = &timelineCreateInfo,
	    .flags = 0};
	const VkResult result = vkCreateSemaphore(rhi.GetDevice(), &semaphoreInfo, nullptr, &m_timelineSemaphore);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    VulkanCommandQueuePolicy::Logger(),
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateSemaphore(timeline)", result));
	}
}

VulkanCommandQueue::~VulkanCommandQueue() noexcept
{
	m_owner.AssertAccess();
	if (m_timelineSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(m_rhi.GetDevice(), m_timelineSemaphore, nullptr);
		m_timelineSemaphore = VK_NULL_HANDLE;
	}
}

RhiSubmissionToken VulkanCommandQueue::Submit(const VulkanQueueSubmission& submission) noexcept
{
	m_owner.AssertAccess();
	if (m_nativeQueue == nullptr || m_nativeQueue->Queue == VK_NULL_HANDLE || submission.CommandBuffer == VK_NULL_HANDLE)
	{
		Diagnostics::Fail(VulkanCommandQueuePolicy::Logger(), __FILE__, __LINE__, "Submit called without a queue or command buffer");
		return {};
	}

	RhiSubmissionState waitState;
	for (const RhiSubmissionToken token : submission.WaitTokens)
	{
		if (!token.IsValid() || token.Queue == m_queueType)
		{
			continue;
		}
		if (!m_rhi.GetCommandQueue(token.Queue).HasSubmitted(token.Value))
		{
			Diagnostics::Fail(
			    VulkanCommandQueuePolicy::Logger(),
			    __FILE__,
			    __LINE__,
			    "Submit rejected a wait for an unsubmitted queue value");
			return {};
		}
		waitState.MarkUsed(token);
	}

	std::array<VkSemaphore, RhiQueueTypeCount + 1> waitSemaphores{};
	std::array<std::uint64_t, RhiQueueTypeCount + 1> waitValues{};
	std::array<VkPipelineStageFlags, RhiQueueTypeCount + 1> waitStages{};
	std::uint32_t waitCount = 0;
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		const ERhiQueueType waitQueueType = static_cast<ERhiQueueType>(queueIndex);
		const RhiSubmissionToken token = waitState.GetToken(waitQueueType);
		if (!token.IsValid())
		{
			continue;
		}

		waitSemaphores[waitCount] = m_rhi.GetCommandQueue(waitQueueType).GetTimelineSemaphore();
		waitValues[waitCount] = token.Value;
		waitStages[waitCount] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		++waitCount;
	}
	if (submission.BinaryWaitSemaphore != VK_NULL_HANDLE)
	{
		waitSemaphores[waitCount] = submission.BinaryWaitSemaphore;
		waitValues[waitCount] = 0;
		waitStages[waitCount] = submission.BinaryWaitStage;
		++waitCount;
	}

	const std::uint64_t submissionValue = m_nextSubmissionValue++;
	const std::array<VkSemaphore, 2> signalSemaphores = {m_timelineSemaphore, submission.BinarySignalSemaphore};
	const std::array<std::uint64_t, 2> signalValues = {submissionValue, 0};
	const std::uint32_t signalCount = submission.BinarySignalSemaphore != VK_NULL_HANDLE ? 2u : 1u;
	const VkTimelineSemaphoreSubmitInfo timelineInfo{
	    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
	    .pNext = nullptr,
	    .waitSemaphoreValueCount = waitCount,
	    .pWaitSemaphoreValues = waitCount != 0 ? waitValues.data() : nullptr,
	    .signalSemaphoreValueCount = signalCount,
	    .pSignalSemaphoreValues = signalValues.data()};
	const VkSubmitInfo submitInfo{
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .pNext = &timelineInfo,
	    .waitSemaphoreCount = waitCount,
	    .pWaitSemaphores = waitCount != 0 ? waitSemaphores.data() : nullptr,
	    .pWaitDstStageMask = waitCount != 0 ? waitStages.data() : nullptr,
	    .commandBufferCount = 1,
	    .pCommandBuffers = &submission.CommandBuffer,
	    .signalSemaphoreCount = signalCount,
	    .pSignalSemaphores = signalSemaphores.data()};

	const VkResult submitResult = vkQueueSubmit(m_nativeQueue->Queue, 1, &submitInfo, VK_NULL_HANDLE);
	if (!VulkanResult::Succeeded(submitResult))
	{
		Diagnostics::Fail(
		    VulkanCommandQueuePolicy::Logger(),
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkQueueSubmit", submitResult));
		return {};
	}

	m_lastSubmittedValue = submissionValue;
	return RhiSubmissionToken{.Queue = m_queueType, .Value = submissionValue};
}

VkResult VulkanCommandQueue::Present(const VkPresentInfoKHR& presentInfo) noexcept
{
	m_owner.AssertAccess();
	if (m_nativeQueue == nullptr || m_nativeQueue->Queue == VK_NULL_HANDLE)
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	return vkQueuePresentKHR(m_nativeQueue->Queue, &presentInfo);
}

void VulkanCommandQueue::WaitForSubmission(std::uint64_t submissionValue) noexcept
{
	m_owner.AssertAccess();
	if (submissionValue == 0)
	{
		return;
	}
	if (!HasSubmitted(submissionValue))
	{
		Diagnostics::Fail(VulkanCommandQueuePolicy::Logger(), __FILE__, __LINE__, "CPU wait rejected an unsubmitted value");
		return;
	}
	if (IsSubmissionComplete(submissionValue))
	{
		return;
	}

	const VkSemaphoreWaitInfo waitInfo{
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .semaphoreCount = 1,
	    .pSemaphores = &m_timelineSemaphore,
	    .pValues = &submissionValue};
	const VkResult result = vkWaitSemaphores(
	    m_rhi.GetDevice(),
	    &waitInfo,
	    VulkanCommandQueuePolicy::GpuWaitTimeoutNanoseconds);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    VulkanCommandQueuePolicy::Logger(),
		    __FILE__,
		    __LINE__,
		    std::format(
		        "{} while waiting for Vulkan {} queue submission {}.",
		        VulkanResult::FormatFailure("vkWaitSemaphores", result),
		        static_cast<std::uint32_t>(m_queueType),
		        submissionValue));
	}
}

bool VulkanCommandQueue::HasSubmitted(std::uint64_t submissionValue) const noexcept
{
	m_owner.AssertAccess();
	if (m_nativeQueue == nullptr)
	{
		return false;
	}
	return submissionValue != 0 && submissionValue <= m_lastSubmittedValue;
}

bool VulkanCommandQueue::IsSubmissionComplete(std::uint64_t submissionValue) const noexcept
{
	m_owner.AssertAccess();
	return submissionValue == 0 || GetCompletedSubmissionValue() >= submissionValue;
}

RhiSubmissionToken VulkanCommandQueue::GetLastSubmittedToken() const noexcept
{
	m_owner.AssertAccess();
	if (m_nativeQueue == nullptr)
	{
		return {};
	}
	return RhiSubmissionToken{.Queue = m_queueType, .Value = m_lastSubmittedValue};
}

std::uint64_t VulkanCommandQueue::GetCompletedSubmissionValue() const noexcept
{
	m_owner.AssertAccess();
	std::uint64_t completedValue = 0;
	const VkResult result = vkGetSemaphoreCounterValue(m_rhi.GetDevice(), m_timelineSemaphore, &completedValue);
	return VulkanResult::Succeeded(result) ? completedValue : 0;
}
