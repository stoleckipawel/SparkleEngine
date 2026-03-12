// ============================================================================
// ScopedEventHandle.h
// ----------------------------------------------------------------------------
// RAII guard that automatically removes subscription on destruction.
//
// USAGE:
//   Event<void(int)> OnChanged;
//   auto handle = OnChanged.Add([](int x) { ... });
//   ScopedEventHandle scoped(OnChanged, handle);
//   // Automatically unsubscribes when scoped goes out of scope
//
// ============================================================================

#pragma once

#include "Core/Public/CoreAPI.h"
#include "EventHandle.h"

#include <functional>
#include <utility>

// Forward declaration of Event template
template <typename Signature, std::size_t Capacity> class Event;

// ============================================================================
// ScopedEventHandle
// ============================================================================

/// RAII guard that automatically removes subscription on destruction.
/// Stores a type-erased cleanup function to avoid template parameter coupling.
class SPARKLE_CORE_API ScopedEventHandle
{
  public:
	ScopedEventHandle() noexcept = default;

	/// Constructs a scoped handle that will unsubscribe on destruction.
	template <typename Signature, std::size_t Capacity>
	ScopedEventHandle(Event<Signature, Capacity>& InEvent, EventHandle InHandle) noexcept :
	    m_Handle(InHandle),
	    m_RemoveFn(
	        [&InEvent, InHandle]()
	        {
		        InEvent.Remove(InHandle);
	        })
	{
	}

	~ScopedEventHandle() { Reset(); }

	// -------------------------------------------------------------------------
	// Move Semantics (Move-Only)
	// -------------------------------------------------------------------------

	ScopedEventHandle(ScopedEventHandle&& Other) noexcept : m_Handle(Other.m_Handle), m_RemoveFn(std::move(Other.m_RemoveFn))
	{
		Other.m_Handle.Invalidate();
		Other.m_RemoveFn = nullptr;
	}

	ScopedEventHandle& operator=(ScopedEventHandle&& Other) noexcept
	{
		if (this != &Other)
		{
			Reset();
			m_Handle = Other.m_Handle;
			m_RemoveFn = std::move(Other.m_RemoveFn);
			Other.m_Handle.Invalidate();
			Other.m_RemoveFn = nullptr;
		}
		return *this;
	}

	// -------------------------------------------------------------------------
	// Deleted Copy Operations
	// -------------------------------------------------------------------------

	ScopedEventHandle(const ScopedEventHandle&) = delete;
	ScopedEventHandle& operator=(const ScopedEventHandle&) = delete;

	// -------------------------------------------------------------------------
	// Public Interface
	// -------------------------------------------------------------------------

	/// Unsubscribes and invalidates this handle. Safe to call multiple times.
	void Reset() noexcept
	{
		if (m_Handle.IsValid() && m_RemoveFn)
		{
			m_RemoveFn();
		}
		m_Handle.Invalidate();
		m_RemoveFn = nullptr;
	}

	/// Returns true if this scoped handle is active.
	bool IsValid() const noexcept { return m_Handle.IsValid(); }

	/// Returns the underlying event handle.
	EventHandle GetHandle() const noexcept { return m_Handle; }

  private:
	EventHandle m_Handle;              ///< Wrapped subscription handle
	std::function<void()> m_RemoveFn;  ///< Type-erased unsubscribe function
};
