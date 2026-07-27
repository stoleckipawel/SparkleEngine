#pragma once

#include "Concurrency/FrameQueue/RenderFramePacket.h"

#include <cstddef>
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
	Retired,
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
	bool Publish(RenderFrameQueueTicket ticket, RenderFramePacket packet);
	bool Consume(RenderFrameQueueTicket ticket, RenderFramePacket& packet);
	bool Retire(RenderFrameQueueTicket ticket);
	bool Cancel(RenderFrameQueueTicket ticket);
	bool WaitUntilReusable(RenderFrameQueueTicket ticket);
	void Close() noexcept;
	void SettleAll() noexcept;

	std::uint32_t GetCapacity() const noexcept { return m_capacity; }
	std::size_t GetFixedStorageBytes() const noexcept;
	bool IsClosed() const noexcept;
	RenderFrameSlotState GetState(std::uint32_t slotIndex) const noexcept;

  private:
	struct Slot final
	{
		RenderFrameSlotState State = RenderFrameSlotState::Free;
		std::uint64_t SequenceNumber = 0;
		std::optional<RenderFramePacket> Packet;
	};

	bool IsTicketCurrentLocked(RenderFrameQueueTicket ticket) const noexcept;
	std::optional<std::uint32_t> FindFreeSlotLocked() const noexcept;

	std::unique_ptr<Slot[]> m_slots;
	std::uint32_t m_capacity = 0;
	mutable std::mutex m_mutex;
	std::condition_variable m_reusable;
	std::uint64_t m_nextSequenceNumber = 1;
	bool m_closed = false;
};
