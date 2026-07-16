#include "PCH.h"

#include "D3D12/Commands/D3D12CommandQueue.h"

#include <format>

namespace
{
	static const auto g_d3d12CommandQueueLogger = Logging::GetOrCreateLogger("RHI.D3D12.Queue");

	const wchar_t* QueueTypeToWideString(ERhiQueueType queueType) noexcept
	{
		switch (queueType)
		{
			case ERhiQueueType::Graphics:
				return L"Graphics";
			case ERhiQueueType::Compute:
				return L"Compute";
			case ERhiQueueType::Copy:
				return L"Copy";
			case ERhiQueueType::Count:
			default:
				return L"Unknown";
		}
	}
}

D3D12CommandQueue::D3D12CommandQueue(
	ID3D12Device& device,
	ERhiQueueType queueType,
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> nativeQueue) noexcept :
	m_queueType(queueType), m_queue(std::move(nativeQueue))
{
	if (m_queue == nullptr)
	{
		const D3D12_COMMAND_QUEUE_DESC queueDesc{
		    .Type = GetNativeCommandListType(queueType),
		    .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		    .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		    .NodeMask = 0};
		CHECK(device.CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_queue.ReleaseAndGetAddressOf())));
	}

	CHECK(device.CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		Diagnostics::Fail(g_d3d12CommandQueueLogger, __FILE__, __LINE__, "Failed to create command queue fence event");
	}

	const std::wstring queueName = std::format(L"Sparkle {} Command Queue", QueueTypeToWideString(queueType));
	const std::wstring fenceName = std::format(L"Sparkle {} Command Queue Fence", QueueTypeToWideString(queueType));
	(void)m_queue->SetName(queueName.c_str());
	(void)m_fence->SetName(fenceName.c_str());
}

D3D12CommandQueue::~D3D12CommandQueue() noexcept
{
	if (m_fenceEvent != nullptr)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
}

D3D12_COMMAND_LIST_TYPE D3D12CommandQueue::GetNativeCommandListType(ERhiQueueType queueType) noexcept
{
	switch (queueType)
	{
		case ERhiQueueType::Graphics:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case ERhiQueueType::Compute:
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case ERhiQueueType::Copy:
			return D3D12_COMMAND_LIST_TYPE_COPY;
		case ERhiQueueType::Count:
		default:
			Diagnostics::Fail(g_d3d12CommandQueueLogger, __FILE__, __LINE__, "Invalid RHI queue type");
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
}

RhiSubmissionToken D3D12CommandQueue::Submit(
	std::span<ID3D12CommandList* const> commandLists,
	std::span<const D3D12QueueWait> waits) noexcept
{
	if (m_queue == nullptr || m_fence == nullptr || commandLists.empty())
	{
		Diagnostics::Fail(g_d3d12CommandQueueLogger, __FILE__, __LINE__, "Submit called without queue submission state");
		return {};
	}

	for (const D3D12QueueWait& wait : waits)
	{
		if (wait.SubmissionValue == 0 || wait.ProducerQueue == this)
		{
			continue;
		}
		if (wait.ProducerQueue == nullptr || !wait.ProducerQueue->HasSubmitted(wait.SubmissionValue))
		{
			Diagnostics::Fail(
			    g_d3d12CommandQueueLogger,
			    __FILE__,
			    __LINE__,
			    "Submit rejected a wait for an unknown or unsubmitted queue value");
			return {};
		}
	}

	std::lock_guard lock(m_submissionMutex);
	for (const D3D12QueueWait& wait : waits)
	{
		if (wait.ProducerQueue != nullptr && wait.ProducerQueue != this && wait.SubmissionValue != 0)
		{
			CHECK(m_queue->Wait(wait.ProducerQueue->m_fence.Get(), wait.SubmissionValue));
		}
	}
	m_queue->ExecuteCommandLists(static_cast<UINT>(commandLists.size()), commandLists.data());
	const std::uint64_t submissionValue = m_nextSubmissionValue++;
	CHECK(m_queue->Signal(m_fence.Get(), submissionValue));
	m_lastSubmittedValue = submissionValue;
	return RhiSubmissionToken{.Queue = m_queueType, .Value = submissionValue};
}

void D3D12CommandQueue::WaitFor(
	const D3D12CommandQueue& executionQueue,
	std::uint64_t submissionValue) noexcept
{
	if (submissionValue == 0 || m_queueType == executionQueue.m_queueType)
	{
		return;
	}
	if (m_queue == nullptr || executionQueue.m_fence == nullptr)
	{
		Diagnostics::Fail(g_d3d12CommandQueueLogger, __FILE__, __LINE__, "Queue wait requested without synchronization state");
		return;
	}
	if (!executionQueue.HasSubmitted(submissionValue))
	{
		Diagnostics::Fail(g_d3d12CommandQueueLogger, __FILE__, __LINE__, "Queue wait rejected an unsubmitted value");
		return;
	}

	std::lock_guard lock(m_submissionMutex);
	CHECK(m_queue->Wait(executionQueue.m_fence.Get(), submissionValue));
}

void D3D12CommandQueue::WaitForSubmission(std::uint64_t submissionValue) noexcept
{
	if (submissionValue == 0)
	{
		return;
	}
	if (!HasSubmitted(submissionValue))
	{
		Diagnostics::Fail(g_d3d12CommandQueueLogger, __FILE__, __LINE__, "CPU wait rejected an unsubmitted value");
		return;
	}
	if (IsSubmissionComplete(submissionValue))
	{
		return;
	}
	if (m_fence == nullptr || m_fenceEvent == nullptr)
	{
		Diagnostics::Fail(g_d3d12CommandQueueLogger, __FILE__, __LINE__, "CPU wait requested without synchronization state");
		return;
	}

	std::lock_guard lock(m_cpuWaitMutex);
	CHECK(m_fence->SetEventOnCompletion(submissionValue, m_fenceEvent));
	WaitForSingleObject(m_fenceEvent, INFINITE);
}

void D3D12CommandQueue::WaitForIdle() noexcept
{
	if (m_queue == nullptr || m_fence == nullptr)
	{
		Diagnostics::Fail(g_d3d12CommandQueueLogger, __FILE__, __LINE__, "Idle wait requested without synchronization state");
		return;
	}

	std::uint64_t submissionValue = 0;
	{
		std::lock_guard lock(m_submissionMutex);
		submissionValue = m_nextSubmissionValue++;
		CHECK(m_queue->Signal(m_fence.Get(), submissionValue));
		m_lastSubmittedValue = submissionValue;
	}
	WaitForSubmission(submissionValue);
}

bool D3D12CommandQueue::HasSubmitted(std::uint64_t submissionValue) const noexcept
{
	std::lock_guard lock(m_submissionMutex);
	return submissionValue != 0 && submissionValue <= m_lastSubmittedValue;
}

bool D3D12CommandQueue::IsSubmissionComplete(std::uint64_t submissionValue) const noexcept
{
	return submissionValue == 0 || (m_fence != nullptr && m_fence->GetCompletedValue() >= submissionValue);
}

RhiSubmissionToken D3D12CommandQueue::GetLastSubmittedToken() const noexcept
{
	std::lock_guard lock(m_submissionMutex);
	return RhiSubmissionToken{.Queue = m_queueType, .Value = m_lastSubmittedValue};
}

std::uint64_t D3D12CommandQueue::GetCompletedSubmissionValue() const noexcept
{
	return m_fence != nullptr ? m_fence->GetCompletedValue() : 0;
}
