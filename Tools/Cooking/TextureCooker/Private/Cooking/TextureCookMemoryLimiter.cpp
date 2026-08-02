#include "Cooking/TextureCookMemoryLimiter.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cassert>
#include <utility>

TextureCookMemoryLimiter::Lease::Lease(TextureCookMemoryLimiter& owner, std::size_t bytes) noexcept :
    m_owner(&owner),
    m_bytes(bytes)
{
}

TextureCookMemoryLimiter::Lease::~Lease()
{
	Release();
}

TextureCookMemoryLimiter::Lease::Lease(Lease&& other) noexcept :
    m_owner(std::exchange(other.m_owner, nullptr)),
    m_bytes(std::exchange(other.m_bytes, 0))
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

TextureCookMemoryLimiter::TextureCookMemoryLimiter(std::size_t capacityBytes) :
    m_capacityBytes(capacityBytes)
{
	if (m_capacityBytes == 0)
	{
		throw Diagnostics::Error("Texture cook memory capacity is zero.");
	}
}

TextureCookMemoryLimiter::Lease TextureCookMemoryLimiter::Acquire(std::size_t bytes)
{
	if (bytes == 0)
	{
		throw Diagnostics::Error("Texture cook pixel-data reservation is zero.");
	}
	if (bytes > m_capacityBytes)
	{
		throw Diagnostics::Error("Texture cook pixel data exceeds the configured memory capacity.");
	}

	std::unique_lock lock(m_mutex);
	m_condition.wait(lock, [this, bytes] { return bytes <= m_capacityBytes - m_usedBytes; });

	m_usedBytes += bytes;
	return Lease(*this, bytes);
}

void TextureCookMemoryLimiter::Release(std::size_t bytes) noexcept
{
	{
		std::lock_guard lock(m_mutex);
		assert(bytes <= m_usedBytes && "Texture cook memory lease released more bytes than it owns.");
		m_usedBytes -= bytes;
	}

	m_condition.notify_all();
}
