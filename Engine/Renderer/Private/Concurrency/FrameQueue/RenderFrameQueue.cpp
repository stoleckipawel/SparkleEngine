#include "PCH.h"
#include "Concurrency/FrameQueue/RenderFrameQueue.h"

#include <limits>

static const auto g_renderFrameQueueLogger = Logging::GetOrCreateLogger("Renderer.FrameQueue");

RenderFrameQueue::RenderFrameQueue(std::uint32_t capacity) :
    m_slots(std::make_unique<Slot[]>(capacity)),
    m_capacity(capacity)
{
	if (m_capacity == 0)
	{
		Diagnostics::Fatal(g_renderFrameQueueLogger, __FILE__, __LINE__, "Render frame queue capacity is zero.");
	}
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
	const std::uint64_t sequenceNumber = IssueSequenceNumberLocked();
	slot.State = RenderFrameSlotState::Writing;
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

bool RenderFrameQueue::WaitUntilReusable(RenderFrameQueueTicket ticket)
{
	std::unique_lock lock(m_mutex);
	if (!IsTicketCurrentLocked(ticket))
	{
		return false;
	}

	m_reusable.wait(
	    lock,
	    [this, ticket]
	    { return m_closed || !IsTicketCurrentLocked(ticket) || m_slots[ticket.SlotIndex].State == RenderFrameSlotState::Free; });
	return !m_closed && IsTicketCurrentLocked(ticket) && m_slots[ticket.SlotIndex].State == RenderFrameSlotState::Free;
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

bool RenderFrameQueue::IsTicketCurrentLocked(RenderFrameQueueTicket ticket) const noexcept
{
	return ticket.IsValid() && ticket.SlotIndex < m_capacity && m_slots[ticket.SlotIndex].SequenceNumber == ticket.SequenceNumber;
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

std::uint64_t RenderFrameQueue::IssueSequenceNumberLocked() noexcept
{
	if (m_nextSequenceNumber == 0)
	{
		Diagnostics::Fatal(g_renderFrameQueueLogger, __FILE__, __LINE__, "Render frame queue sequence identity exhausted.");
	}

	const std::uint64_t sequenceNumber = m_nextSequenceNumber;
	m_nextSequenceNumber = sequenceNumber == (std::numeric_limits<std::uint64_t>::max)() ? 0 : sequenceNumber + 1;
	return sequenceNumber;
}
