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
	m_reusable.wait(lock, [this] { return m_closed || FindFreeSlotLocked().has_value(); });
	if (m_closed)
	{
		return std::nullopt;
	}

	const std::uint32_t slotIndex = *FindFreeSlotLocked();
	Slot& slot = m_slots[slotIndex];
	slot.State = RenderFrameSlotState::Writing;
	const std::uint64_t sequenceNumber = m_nextSequenceNumber++;
	slot.SequenceNumber = sequenceNumber;
	return RenderFrameQueueTicket{slotIndex, sequenceNumber};
}

bool RenderFrameQueue::Publish(RenderFrameQueueTicket ticket, RenderFramePacket packet)
{
	std::lock_guard lock(m_mutex);
	if (!IsTicketCurrentLocked(ticket))
	{
		return false;
	}

	Slot& slot = m_slots[ticket.SlotIndex];
	if (slot.State != RenderFrameSlotState::Writing)
	{
		return false;
	}

	slot.Packet.emplace(std::move(packet));
	slot.State = RenderFrameSlotState::Ready;
	return true;
}

bool RenderFrameQueue::Consume(RenderFrameQueueTicket ticket, RenderFramePacket& packet)
{
	std::lock_guard lock(m_mutex);
	if (!IsTicketCurrentLocked(ticket))
	{
		return false;
	}

	Slot& slot = m_slots[ticket.SlotIndex];
	if (slot.State != RenderFrameSlotState::Ready || !slot.Packet)
	{
		return false;
	}

	slot.State = RenderFrameSlotState::Rendering;
	packet = std::move(*slot.Packet);
	slot.Packet.reset();
	return true;
}

bool RenderFrameQueue::Retire(RenderFrameQueueTicket ticket)
{
	{
		std::lock_guard lock(m_mutex);
		if (!IsTicketCurrentLocked(ticket))
		{
			return false;
		}

		Slot& slot = m_slots[ticket.SlotIndex];
		if (slot.State != RenderFrameSlotState::Rendering)
		{
			return false;
		}

		slot.State = RenderFrameSlotState::Free;
	}

	m_reusable.notify_all();
	return true;
}

bool RenderFrameQueue::Cancel(RenderFrameQueueTicket ticket)
{
	{
		std::lock_guard lock(m_mutex);
		if (!IsTicketCurrentLocked(ticket))
		{
			return false;
		}

		Slot& slot = m_slots[ticket.SlotIndex];
		if (slot.State != RenderFrameSlotState::Writing &&
		    slot.State != RenderFrameSlotState::Ready)
		{
			return false;
		}

		slot.Packet.reset();
		slot.State = RenderFrameSlotState::Free;
	}

	m_reusable.notify_all();
	return true;
}

bool RenderFrameQueue::WaitUntilReusable(RenderFrameQueueTicket ticket)
{
	std::unique_lock lock(m_mutex);
	m_reusable.wait(lock, [this, ticket]
	{
		return m_closed || !IsTicketCurrentLocked(ticket) ||
		       m_slots[ticket.SlotIndex].State == RenderFrameSlotState::Free;
	});
	return !IsTicketCurrentLocked(ticket) ||
	       m_slots[ticket.SlotIndex].State == RenderFrameSlotState::Free;
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
	{
		std::lock_guard lock(m_mutex);
		m_closed = true;
		for (std::uint32_t slotIndex = 0; slotIndex < m_capacity; ++slotIndex)
		{
			Slot& slot = m_slots[slotIndex];
			slot.Packet.reset();
			slot.State = RenderFrameSlotState::Free;
		}
	}
	m_reusable.notify_all();
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
	std::lock_guard lock(m_mutex);
	return slotIndex < m_capacity ? m_slots[slotIndex].State : RenderFrameSlotState::Retired;
}

bool RenderFrameQueue::IsTicketCurrentLocked(RenderFrameQueueTicket ticket) const noexcept
{
	return ticket.IsValid() && ticket.SlotIndex < m_capacity &&
	       m_slots[ticket.SlotIndex].SequenceNumber == ticket.SequenceNumber;
}

std::optional<std::uint32_t> RenderFrameQueue::FindFreeSlotLocked() const noexcept
{
	for (std::uint32_t slotIndex = 0; slotIndex < m_capacity; ++slotIndex)
	{
		if (m_slots[slotIndex].State == RenderFrameSlotState::Free)
		{
			return slotIndex;
		}
	}

	return std::nullopt;
}
