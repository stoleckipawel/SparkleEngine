#pragma once

#include "Concurrency/FrameQueue/RenderExecutionRequest.h"

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

enum class RenderFrameSlotState : std::uint8_t
{
	Free,
	Writing,
	Ready,
	Rendering,
};

struct RenderFrameQueueTicket final
{
	std::uint32_t SlotIndex = 0;
	std::uint64_t SequenceNumber = 0;

	constexpr bool IsValid() const noexcept { return SequenceNumber != 0; }
};

class RenderFrameQueue final
{
public:
	explicit RenderFrameQueue(std::uint32_t capacity);

	RenderFrameQueue(const RenderFrameQueue&) = delete;
	RenderFrameQueue& operator=(const RenderFrameQueue&) = delete;

	std::optional<RenderFrameQueueTicket> Acquire();
	bool Publish(RenderFrameQueueTicket ticket, RenderExecutionRequest request);
	bool Consume(RenderFrameQueueTicket ticket, RenderExecutionRequest& request);
	bool Retire(RenderFrameQueueTicket ticket);
	bool WaitUntilReusable(RenderFrameQueueTicket ticket);
	void Close() noexcept;
	void SettleAll() noexcept;

private:
	struct Slot final
	{
		RenderFrameSlotState State = RenderFrameSlotState::Free;
		std::uint64_t SequenceNumber = 0;
		std::optional<RenderExecutionRequest> Request;
	};

	bool IsTicketCurrentLocked(RenderFrameQueueTicket ticket) const noexcept;
	std::optional<std::uint32_t> FindFreeSlotLocked() const noexcept;
	std::uint64_t IssueSequenceNumberLocked() noexcept;

	std::unique_ptr<Slot[]> m_slots;
	std::uint32_t m_capacity = 0;
	std::mutex m_mutex;
	std::condition_variable m_reusable;
	std::uint64_t m_nextSequenceNumber = 1;
	bool m_closed = false;
};
