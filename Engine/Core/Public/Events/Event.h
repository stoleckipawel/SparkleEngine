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

	// Non-copyable, non-movable (prevents handle invalidation)
	Event(const Event&) = delete;
	Event& operator=(const Event&) = delete;
	Event(Event&&) = delete;
	Event& operator=(Event&&) = delete;

	// =========================================================================
	// Subscription Management
	// =========================================================================

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

		// Capacity exceeded
		assert(false && "Event capacity exceeded. Increase template Capacity parameter.");
		return EventHandle{};
	}

	/// Removes a listener by handle. No-op if handle is invalid or not found.
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

	/// Removes all listeners.
	void Clear() noexcept
	{
		for (std::size_t i = 0; i < Capacity; ++i)
		{
			m_Entries[i].Handle.Invalidate();
			m_Entries[i].Callback = nullptr;
		}
	}

	// =========================================================================
	// Broadcasting
	// =========================================================================

	/// Invokes all registered listeners with the given arguments.
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

	// =========================================================================
	// Queries
	// =========================================================================

	/// Returns true if any listeners are registered.
	bool IsBound() const noexcept
	{
		for (std::size_t i = 0; i < Capacity; ++i)
		{
			if (m_Entries[i].Handle.IsValid())
				return true;
		}
		return false;
	}

	/// Returns the number of active subscriptions.
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

	/// Returns the maximum number of subscriptions this event can hold.
	static constexpr std::size_t GetCapacity() noexcept { return Capacity; }

  private:
	// -------------------------------------------------------------------------
	// Internal Storage
	// -------------------------------------------------------------------------

	struct Entry
	{
		EventHandle Handle;     ///< Subscription identifier
		CallbackType Callback;  ///< Listener function
	};

	std::array<Entry, Capacity> m_Entries{};  ///< Fixed-size listener storage
	std::uint32_t m_NextId = 0;               ///< Counter for unique handle IDs
};
