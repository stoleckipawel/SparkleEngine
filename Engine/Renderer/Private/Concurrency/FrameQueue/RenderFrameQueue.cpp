#include "PCH.h"
#include "Concurrency/FrameQueue/RenderFrameQueue.h"

#include <algorithm>

RenderFrameQueue::RenderFrameQueue(std::uint32_t capacity) :
    m_slots(std::make_unique<Slot[]>((std::max)(capacity, 1u))),
    m_capacity((std::max)(capacity, 1u))
{
}

std::optional<RenderFrameQueueTicket> RenderFrameQueue::Acquire()
{
	std::unique_lock lock(m_mutex);
	m_reusable.wait(lock, [this] { return m_closed || FindFreeSlot().has_value(); });
	if (m_closed) return std::nullopt;

	const std::uint32_t slotIndex = *FindFreeSlot();
	Slot& slot = m_slots[slotIndex];
	slot.State.store(RenderFrameSlotState::Writing, std::memory_order_relaxed);
	const std::uint64_t sequenceNumber = m_nextSequenceNumber++;
	slot.SequenceNumber.store(sequenceNumber, std::memory_order_relaxed);
	return RenderFrameQueueTicket{slotIndex, sequenceNumber};
}

bool RenderFrameQueue::Publish(RenderFrameQueueTicket ticket, RenderFramePacket packet)
{
	if (!IsTicketCurrent(ticket)) return false;
	Slot& slot = m_slots[ticket.SlotIndex];
	if (slot.State.load(std::memory_order_relaxed) != RenderFrameSlotState::Writing) return false;
	slot.Packet.emplace(std::move(packet));
	slot.State.store(RenderFrameSlotState::Ready, std::memory_order_release);
	return true;
}

bool RenderFrameQueue::Consume(RenderFrameQueueTicket ticket, RenderFramePacket& packet)
{
	if (!IsTicketCurrent(ticket)) return false;
	Slot& slot = m_slots[ticket.SlotIndex];
	RenderFrameSlotState expected = RenderFrameSlotState::Ready;
	if (!slot.State.compare_exchange_strong(
	        expected,
	        RenderFrameSlotState::Rendering,
	        std::memory_order_acquire,
	        std::memory_order_relaxed))
		return false;
	if (!slot.Packet) return false;
	packet = std::move(*slot.Packet);
	slot.Packet.reset();
	return true;
}

bool RenderFrameQueue::Retire(RenderFrameQueueTicket ticket)
{
	if (!IsTicketCurrent(ticket)) return false;
	Slot& slot = m_slots[ticket.SlotIndex];
	RenderFrameSlotState expected = RenderFrameSlotState::Rendering;
	if (!slot.State.compare_exchange_strong(
	        expected,
	        RenderFrameSlotState::Retired,
	        std::memory_order_release,
	        std::memory_order_relaxed))
		return false;
	slot.State.store(RenderFrameSlotState::Free, std::memory_order_release);
	m_reusable.notify_all();
	return true;
}

bool RenderFrameQueue::Cancel(RenderFrameQueueTicket ticket)
{
	if (!IsTicketCurrent(ticket)) return false;
	Slot& slot = m_slots[ticket.SlotIndex];
	const RenderFrameSlotState state = slot.State.load(std::memory_order_acquire);
	if (state == RenderFrameSlotState::Free || state == RenderFrameSlotState::Retired)
	{
		return false;
	}

	slot.State.store(RenderFrameSlotState::Retired, std::memory_order_release);
	slot.Packet.reset();
	slot.State.store(RenderFrameSlotState::Free, std::memory_order_release);
	m_reusable.notify_all();
	return true;
}

bool RenderFrameQueue::WaitUntilReusable(RenderFrameQueueTicket ticket)
{
	std::unique_lock lock(m_mutex);
	m_reusable.wait(lock, [this, ticket]
	{
		return m_closed || !IsTicketCurrent(ticket) ||
		       m_slots[ticket.SlotIndex].State.load(std::memory_order_acquire) == RenderFrameSlotState::Free;
	});
	return !IsTicketCurrent(ticket) ||
	       m_slots[ticket.SlotIndex].State.load(std::memory_order_acquire) == RenderFrameSlotState::Free;
}

void RenderFrameQueue::Close() noexcept
{
	{
		std::lock_guard lock(m_mutex);
		m_closed = true;
	}
	m_reusable.notify_all();
}

void RenderFrameQueue::SettleAll() noexcept
{
	Close();
	for (std::uint32_t slotIndex = 0; slotIndex < m_capacity; ++slotIndex)
	{
		const std::uint64_t sequenceNumber = m_slots[slotIndex].SequenceNumber.load(std::memory_order_acquire);
		if (sequenceNumber != 0)
		{
			(void) Cancel(RenderFrameQueueTicket{slotIndex, sequenceNumber});
		}
	}
}

bool RenderFrameQueue::IsClosed() const noexcept
{
	std::lock_guard lock(m_mutex);
	return m_closed;
}

std::size_t RenderFrameQueue::GetFixedStorageBytes() const noexcept
{
	return sizeof(Slot) * m_capacity;
}

RenderFrameSlotState RenderFrameQueue::GetState(std::uint32_t slotIndex) const noexcept
{
	return slotIndex < m_capacity ? m_slots[slotIndex].State.load(std::memory_order_acquire)
	                              : RenderFrameSlotState::Retired;
}

bool RenderFrameQueue::IsTicketCurrent(RenderFrameQueueTicket ticket) const noexcept
{
	return ticket.IsValid() && ticket.SlotIndex < m_capacity &&
	       m_slots[ticket.SlotIndex].SequenceNumber.load(std::memory_order_relaxed) == ticket.SequenceNumber;
}

std::optional<std::uint32_t> RenderFrameQueue::FindFreeSlot() const noexcept
{
	for (std::uint32_t slotIndex = 0; slotIndex < m_capacity; ++slotIndex)
		if (m_slots[slotIndex].State.load(std::memory_order_acquire) == RenderFrameSlotState::Free)
			return slotIndex;
	return std::nullopt;
}
