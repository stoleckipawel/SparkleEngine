#pragma once

#include "Core/Public/CoreAPI.h"
#include "EventHandle.h"

#include <functional>
#include <utility>

template <typename Signature, std::size_t Capacity> class Event;
class SPARKLE_CORE_API ScopedEventHandle
{
  public:
	ScopedEventHandle() noexcept = default;

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

	ScopedEventHandle(const ScopedEventHandle&) = delete;
	ScopedEventHandle& operator=(const ScopedEventHandle&) = delete;

	void Reset() noexcept
	{
		if (m_Handle.IsValid() && m_RemoveFn)
		{
			m_RemoveFn();
		}
		m_Handle.Invalidate();
		m_RemoveFn = nullptr;
	}

	bool IsValid() const noexcept { return m_Handle.IsValid(); }

	EventHandle GetHandle() const noexcept { return m_Handle; }

  private:
	EventHandle m_Handle;
	std::function<void()> m_RemoveFn;
};
