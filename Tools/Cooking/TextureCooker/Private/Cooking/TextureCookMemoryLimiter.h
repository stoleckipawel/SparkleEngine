#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>

class TextureCookMemoryLimiter final
{
public:
	class Lease final
	{
	public:
		Lease() noexcept = default;
		~Lease();
		Lease(Lease&& other) noexcept;
		Lease& operator=(Lease&& other) noexcept;
		Lease(const Lease&) = delete;
		Lease& operator=(const Lease&) = delete;

	private:
		friend class TextureCookMemoryLimiter;
		Lease(TextureCookMemoryLimiter& owner, std::size_t bytes) noexcept;
		void Release() noexcept;
		TextureCookMemoryLimiter* m_owner = nullptr;
		std::size_t m_bytes = 0;
	};

	explicit TextureCookMemoryLimiter(std::size_t capacityBytes);
	Lease Acquire(std::size_t bytes);

private:
	void Release(std::size_t bytes) noexcept;

	const std::size_t m_capacityBytes;
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	std::size_t m_usedBytes = 0;
};
