#pragma once

#include "Commands/RhiQueue.h"
#include "Core/Public/Threading/ThreadOwnership.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>
#include <span>

class D3D12CommandQueue;

struct D3D12QueueWait final
{
	const D3D12CommandQueue* ProducerQueue = nullptr;
	std::uint64_t SubmissionValue = 0;
};

class D3D12CommandQueue final
{
public:
	D3D12CommandQueue(ID3D12Device& device, ERhiQueueType queueType, Microsoft::WRL::ComPtr<ID3D12CommandQueue> nativeQueue = {}) noexcept;
	~D3D12CommandQueue() noexcept;

	D3D12CommandQueue(const D3D12CommandQueue&) = delete;
	D3D12CommandQueue& operator=(const D3D12CommandQueue&) = delete;
	D3D12CommandQueue(D3D12CommandQueue&&) = delete;
	D3D12CommandQueue& operator=(D3D12CommandQueue&&) = delete;

	static D3D12_COMMAND_LIST_TYPE GetNativeCommandListType(ERhiQueueType queueType) noexcept;

	RhiSubmissionToken Submit(std::span<ID3D12CommandList* const> commandLists, std::span<const D3D12QueueWait> waits = {}) noexcept;
	RhiSubmissionToken Signal() noexcept;
	void WaitFor(const D3D12CommandQueue& executionQueue, std::uint64_t submissionValue) noexcept;
	void WaitForSubmission(std::uint64_t submissionValue) noexcept;
	void WaitForIdle() noexcept;
	bool HasSubmitted(std::uint64_t submissionValue) const noexcept;
	bool IsSubmissionComplete(std::uint64_t submissionValue) const noexcept;

	RhiSubmissionToken GetLastSubmittedToken() const noexcept;
	std::uint64_t GetCompletedSubmissionValue() const noexcept;
	ERhiQueueType GetQueueType() const noexcept { return m_queueType; }
	const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetNativeQueue() const noexcept
	{
		m_owner.AssertAccess();
		return m_queue;
	}
	const Microsoft::WRL::ComPtr<ID3D12Fence1>& GetFence() const noexcept
	{
		m_owner.AssertAccess();
		return m_fence;
	}
	HANDLE GetFenceEvent() const noexcept
	{
		m_owner.AssertAccess();
		return m_fenceEvent;
	}

private:
	Threading::OwnerThread m_owner{"D3D12 command queue"};
	ERhiQueueType m_queueType = ERhiQueueType::Graphics;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue;
	Microsoft::WRL::ComPtr<ID3D12Fence1> m_fence;
	HANDLE m_fenceEvent = nullptr;
	std::uint64_t m_nextSubmissionValue = 1;
	std::uint64_t m_lastSubmittedValue = 0;
};
