#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <stop_token>

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
		bool IsValid() const noexcept { return m_owner != nullptr; }

	  private:
		friend class TextureCookMemoryLimiter;
		Lease(TextureCookMemoryLimiter& owner, std::size_t bytes) noexcept : m_owner(&owner), m_bytes(bytes) {}
		void Release() noexcept;
		TextureCookMemoryLimiter* m_owner = nullptr;
		std::size_t m_bytes = 0;
	};

	explicit TextureCookMemoryLimiter(std::size_t capacityBytes);
	Lease Acquire(std::size_t bytes, std::stop_token cancellation);
	std::size_t GetPeakBytes() const noexcept;

  private:
	void Release(std::size_t bytes) noexcept;

	const std::size_t m_capacityBytes;
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	std::size_t m_usedBytes = 0;
	std::size_t m_peakBytes = 0;
};
