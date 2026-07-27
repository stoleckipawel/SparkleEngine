#include "Cooking/TextureCookMemoryLimiter.h"

#include <algorithm>
#include <stop_token>
#include <utility>

TextureCookMemoryLimiter::Lease::Lease(TextureCookMemoryLimiter& owner, std::size_t bytes) noexcept :
    m_owner(&owner), m_bytes(bytes)
{
}

TextureCookMemoryLimiter::Lease::~Lease()
{
	Release();
}

TextureCookMemoryLimiter::Lease::Lease(Lease&& other) noexcept :
    m_owner(std::exchange(other.m_owner, nullptr)), m_bytes(std::exchange(other.m_bytes, 0))
{
}

TextureCookMemoryLimiter::Lease& TextureCookMemoryLimiter::Lease::operator=(Lease&& other) noexcept
{
	if (this != &other)
	{
		Release();
		m_owner = std::exchange(other.m_owner, nullptr);
		m_bytes = std::exchange(other.m_bytes, 0);
	}
	return *this;
}

void TextureCookMemoryLimiter::Lease::Release() noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->Release(m_bytes);
		m_owner = nullptr;
		m_bytes = 0;
	}
}

TextureCookMemoryLimiter::TextureCookMemoryLimiter(std::size_t capacityBytes) : m_capacityBytes(std::max<std::size_t>(capacityBytes, 1)) {}

TextureCookMemoryLimiter::Lease TextureCookMemoryLimiter::Acquire(std::size_t bytes, std::stop_token cancellation)
{
	const std::size_t weight = std::clamp<std::size_t>(bytes, 1, m_capacityBytes);
	std::stop_callback cancellationWake(
	    cancellation,
	    [this]
	    {
		    m_condition.notify_all();
	    });
	std::unique_lock lock(m_mutex);
	m_condition.wait(
	    lock,
	    [this, weight, cancellation]
	    {
		    return cancellation.stop_requested() || weight <= m_capacityBytes - m_usedBytes;
	    });
	if (cancellation.stop_requested())
		return {};
	m_usedBytes += weight;
	return Lease(*this, weight);
}

void TextureCookMemoryLimiter::Release(std::size_t bytes) noexcept
{
	{
		std::lock_guard lock(m_mutex);
		m_usedBytes -= std::min(bytes, m_usedBytes);
	}
	m_condition.notify_all();
}
