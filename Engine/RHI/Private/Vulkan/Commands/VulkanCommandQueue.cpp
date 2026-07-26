#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanCommandQueue.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"

#include <array>
#include <format>

struct VulkanCommandQueue::NativeSubmission final
{
	std::array<VkSemaphore, RhiQueueTypeCount + 1> WaitSemaphores;
	std::array<std::uint64_t, RhiQueueTypeCount + 1> WaitValues;
	std::array<VkPipelineStageFlags, RhiQueueTypeCount + 1> WaitStages;
	std::array<VkSemaphore, 2> SignalSemaphores;
	std::array<std::uint64_t, 2> SignalValues;
	VkTimelineSemaphoreSubmitInfo TimelineInfo = {};
	VkSubmitInfo SubmitInfo = {};
	std::uint32_t WaitCount = 0;
	std::uint32_t SignalCount = 0;
};

const std::shared_ptr<spdlog::logger>& VulkanCommandQueue::GetLogger()
{
	static const auto logger = Logging::GetOrCreateLogger("RHI.Vulkan.Queue");
	return logger;
}

std::uint64_t VulkanCommandQueue::GetWaitTimeoutNanoseconds() noexcept
{
	#if SPARKLE_BUILD_SHIPPING
	return UINT64_MAX;
	#else
	return 30'000'000'000ull;
	#endif
}

VulkanCommandQueue::VulkanCommandQueue(
	VulkanRhi& rhi,
	ERhiQueueType queueType,
	std::shared_ptr<VulkanNativeQueue> nativeQueue) noexcept :
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
		    GetLogger(),
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
		Diagnostics::Fail(GetLogger(), __FILE__, __LINE__, "Submit called without a queue or command buffer");
		return {};
	}

	RhiSubmissionState waitState;
	if (!ResolveWaitState(submission.WaitTokens, waitState))
	{
		return {};
	}

	const std::uint64_t submissionValue = m_nextSubmissionValue++;
	NativeSubmission nativeSubmission;
	BuildNativeSubmission(submission, waitState, submissionValue, nativeSubmission);

	const VkResult submitResult = SubmitNative(nativeSubmission.SubmitInfo);
	if (!VulkanResult::Succeeded(submitResult))
	{
		Diagnostics::Fail(
		    GetLogger(),
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkQueueSubmit", submitResult));
		return {};
	}

	m_lastSubmittedValue = submissionValue;
	return RhiSubmissionToken{.Queue = m_queueType, .Value = submissionValue};
}

bool VulkanCommandQueue::ResolveWaitState(
    std::span<const RhiSubmissionToken> waitTokens,
    RhiSubmissionState& waitState) const noexcept
{
	for (const RhiSubmissionToken token : waitTokens)
	{
		if (!token.IsValid() || token.Queue == m_queueType)
		{
			continue;
		}
		if (!m_rhi.GetCommandQueue(token.Queue).HasSubmitted(token.Value))
		{
			Diagnostics::Fail(
			    GetLogger(),
			    __FILE__,
			    __LINE__,
			    "Submit rejected a wait for an unsubmitted queue value");
			return false;
		}

		waitState.MarkUsed(token);
	}

	return true;
}

void VulkanCommandQueue::BuildNativeSubmission(
    const VulkanQueueSubmission& submission,
    const RhiSubmissionState& waitState,
    std::uint64_t submissionValue,
    NativeSubmission& nativeSubmission) const noexcept
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		const ERhiQueueType waitQueueType = static_cast<ERhiQueueType>(queueIndex);
		const RhiSubmissionToken token = waitState.GetToken(waitQueueType);
		if (!token.IsValid())
		{
			continue;
		}

		nativeSubmission.WaitSemaphores[nativeSubmission.WaitCount] =
		    m_rhi.GetCommandQueue(waitQueueType).GetTimelineSemaphore();
		nativeSubmission.WaitValues[nativeSubmission.WaitCount] = token.Value;
		nativeSubmission.WaitStages[nativeSubmission.WaitCount] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		++nativeSubmission.WaitCount;
	}
	if (submission.BinaryWaitSemaphore != VK_NULL_HANDLE)
	{
		nativeSubmission.WaitSemaphores[nativeSubmission.WaitCount] = submission.BinaryWaitSemaphore;
		nativeSubmission.WaitValues[nativeSubmission.WaitCount] = 0;
		nativeSubmission.WaitStages[nativeSubmission.WaitCount] = submission.BinaryWaitStage;
		++nativeSubmission.WaitCount;
	}

	nativeSubmission.SignalSemaphores = {m_timelineSemaphore, submission.BinarySignalSemaphore};
	nativeSubmission.SignalValues = {submissionValue, 0};
	nativeSubmission.SignalCount = submission.BinarySignalSemaphore != VK_NULL_HANDLE ? 2u : 1u;

	nativeSubmission.TimelineInfo = VkTimelineSemaphoreSubmitInfo{
	    .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
	    .pNext = nullptr,
	    .waitSemaphoreValueCount = nativeSubmission.WaitCount,
	    .pWaitSemaphoreValues =
	        nativeSubmission.WaitCount != 0 ? nativeSubmission.WaitValues.data() : nullptr,
	    .signalSemaphoreValueCount = nativeSubmission.SignalCount,
	    .pSignalSemaphoreValues = nativeSubmission.SignalValues.data()};
	nativeSubmission.SubmitInfo = VkSubmitInfo{
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .pNext = &nativeSubmission.TimelineInfo,
	    .waitSemaphoreCount = nativeSubmission.WaitCount,
	    .pWaitSemaphores =
	        nativeSubmission.WaitCount != 0 ? nativeSubmission.WaitSemaphores.data() : nullptr,
	    .pWaitDstStageMask =
	        nativeSubmission.WaitCount != 0 ? nativeSubmission.WaitStages.data() : nullptr,
	    .commandBufferCount = 1,
	    .pCommandBuffers = &submission.CommandBuffer,
	    .signalSemaphoreCount = nativeSubmission.SignalCount,
	    .pSignalSemaphores = nativeSubmission.SignalSemaphores.data()};
}

VkResult VulkanCommandQueue::SubmitNative(const VkSubmitInfo& submission) noexcept
{
	std::scoped_lock lock(m_nativeQueue->SubmissionMutex);
	return vkQueueSubmit(m_nativeQueue->Queue, 1, &submission, VK_NULL_HANDLE);
}

VkResult VulkanCommandQueue::Present(const VkPresentInfoKHR& presentInfo) noexcept
{
	m_owner.AssertAccess();
	if (m_nativeQueue == nullptr || m_nativeQueue->Queue == VK_NULL_HANDLE)
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	std::scoped_lock lock(m_nativeQueue->SubmissionMutex);
	return vkQueuePresentKHR(m_nativeQueue->Queue, &presentInfo);
}

void VulkanCommandQueue::DrainForSwapChainRecreation() noexcept
{
	m_owner.AssertAccess();
	if (m_nativeQueue == nullptr || m_nativeQueue->Queue == VK_NULL_HANDLE)
	{
		return;
	}

	VkResult result = VK_ERROR_INITIALIZATION_FAILED;
	{
		std::scoped_lock lock(m_nativeQueue->SubmissionMutex);
		result = vkQueueWaitIdle(m_nativeQueue->Queue);
	}

	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    GetLogger(),
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure(
		        "vkQueueWaitIdle",
		        result));
	}
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
		Diagnostics::Fail(GetLogger(), __FILE__, __LINE__, "CPU wait rejected an unsubmitted value");
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
	    GetWaitTimeoutNanoseconds());
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    GetLogger(),
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

VkQueue VulkanCommandQueue::GetNativeQueue() const noexcept
{
	m_owner.AssertAccess();
	return m_nativeQueue != nullptr ? m_nativeQueue->Queue : VK_NULL_HANDLE;
}

VkSemaphore VulkanCommandQueue::GetTimelineSemaphore() const noexcept
{
	m_owner.AssertAccess();
	return m_timelineSemaphore;
}
