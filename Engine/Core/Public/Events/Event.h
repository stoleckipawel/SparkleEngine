#pragma once

#include "EventHandle.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <utility>

template <typename Signature, std::size_t Capacity = 8> class Event;
template <typename... Args, std::size_t Capacity> class Event<void(Args...), Capacity>
{
  public:
	using CallbackType = std::function<void(Args...)>;

	Event() noexcept = default;
	~Event() = default;

	Event(const Event&) = delete;
	Event& operator=(const Event&) = delete;
	Event(Event&&) = delete;
	Event& operator=(Event&&) = delete;

	EventHandle Add(CallbackType Callback) noexcept
	{
		for (std::size_t i = 0; i < Capacity; ++i)
		{
			if (!m_Entries[i].Handle.IsValid())
			{
				m_Entries[i].Handle.Id = ++m_NextId;
				m_Entries[i].Callback = std::move(Callback);
				return m_Entries[i].Handle;
			}
		}

		assert(false && "Event capacity exceeded. Increase template Capacity parameter.");
		return EventHandle{};
	}

	void Remove(EventHandle Handle) noexcept
	{
		if (!Handle.IsValid())
			return;

		for (std::size_t i = 0; i < Capacity; ++i)
		{
			if (m_Entries[i].Handle == Handle)
			{
				m_Entries[i].Handle.Invalidate();
				m_Entries[i].Callback = nullptr;
				return;
			}
		}
	}

	void Clear() noexcept
	{
		for (std::size_t i = 0; i < Capacity; ++i)
		{
			m_Entries[i].Handle.Invalidate();
			m_Entries[i].Callback = nullptr;
		}
	}

	void Broadcast(Args... InArgs) const noexcept
	{
		for (std::size_t i = 0; i < Capacity; ++i)
		{
			if (m_Entries[i].Handle.IsValid() && m_Entries[i].Callback)
			{
				m_Entries[i].Callback(InArgs...);
			}
		}
	}

	bool IsBound() const noexcept
	{
		for (std::size_t i = 0; i < Capacity; ++i)
		{
			if (m_Entries[i].Handle.IsValid())
				return true;
		}
		return false;
	}

	std::size_t GetBoundCount() const noexcept
	{
		std::size_t Count = 0;
		for (std::size_t i = 0; i < Capacity; ++i)
		{
			if (m_Entries[i].Handle.IsValid())
				++Count;
		}
		return Count;
	}

	static constexpr std::size_t GetCapacity() noexcept { return Capacity; }

  private:
	struct Entry
	{
		EventHandle Handle;
		CallbackType Callback;
	};

	std::array<Entry, Capacity> m_Entries{};
	std::uint32_t m_NextId = 0;
};
