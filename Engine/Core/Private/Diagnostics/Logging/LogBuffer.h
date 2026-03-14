#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace Logging
{
	class Buffer final
	{
	  public:
		void Append(const char* data, std::size_t len) noexcept
		{
			if (!data || len == 0)
			{
				return;
			}

			const std::size_t count = (std::min)(len, Remaining());
			if (count == 0)
			{
				return;
			}

			std::memcpy(m_data + m_pos, data, count);
			m_pos += count;
		}

		void Append(std::string_view text) noexcept
		{
			Append(text.data(), text.size());
		}

		void Append(const char* text) noexcept
		{
			if (text)
			{
				Append(text, std::strlen(text));
			}
		}

		template <typename... Args> void Format(const char* fmt, Args... args) noexcept
		{
			const std::size_t space = Remaining();
			if (space == 0)
			{
				return;
			}

			const int written = std::snprintf(m_data + m_pos, space + 1, fmt, args...);
			if (written <= 0)
			{
				return;
			}

			m_pos += (std::min)(static_cast<std::size_t>(written), space);
		}

		void Newline() noexcept
		{
			if (Remaining() > 0)
			{
				m_data[m_pos++] = '\n';
			}
		}

		const char* CStr() noexcept
		{
			Terminate();
			return m_data;
		}

		const char* Data() const noexcept { return m_data; }
		std::size_t Size() const noexcept { return m_pos; }

	  private:
		std::size_t Remaining() const noexcept
		{
			return m_pos < (kCapacity - 1) ? (kCapacity - 1 - m_pos) : 0;
		}

		void Terminate() noexcept
		{
			m_data[m_pos < kCapacity ? m_pos : (kCapacity - 1)] = '\0';
		}

		static constexpr std::size_t kCapacity = 2048;
		char m_data[kCapacity]{};
		std::size_t m_pos = 0;
	};
}