#pragma once

#include <atomic>
#include <cstddef>
#include <exception>

namespace Assets
{
	class SceneLoadBudget final
	{
	public:
		static constexpr std::size_t MaximumRetainedBytes = 512ull * 1024ull * 1024ull;

		explicit SceneLoadBudget(std::size_t maximumBytes = MaximumRetainedBytes) noexcept :
		    m_maximumBytes(maximumBytes)
		{
		}

		bool TryReserve(std::size_t amount) noexcept
		{
			if (amount > m_maximumBytes)
			{
				return false;
			}

			std::size_t retainedBytes = m_retainedBytes.load(std::memory_order_relaxed);
			while (retainedBytes <= m_maximumBytes - amount)
			{
				if (m_retainedBytes
				        .compare_exchange_weak(retainedBytes, retainedBytes + amount, std::memory_order_acq_rel, std::memory_order_relaxed))
				{
					return true;
				}
			}
			return false;
		}

		void Release(std::size_t amount) noexcept
		{
			const std::size_t previous = m_retainedBytes.fetch_sub(amount, std::memory_order_acq_rel);
			if (previous < amount)
			{
				std::terminate();
			}
		}

		std::size_t GetMaximumBytes() const noexcept { return m_maximumBytes; }
		std::size_t GetRetainedBytes() const noexcept { return m_retainedBytes.load(std::memory_order_acquire); }

	private:
		const std::size_t m_maximumBytes;
		std::atomic<std::size_t> m_retainedBytes = 0;
	};
}
