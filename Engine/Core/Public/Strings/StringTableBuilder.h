#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace Strings
{
	struct StringTableEntry final
	{
		std::uint32_t OffsetInBytes = 0;
		std::uint32_t SizeInBytes = 0;

		constexpr bool IsValid() const noexcept { return SizeInBytes > 0; }
		explicit constexpr operator bool() const noexcept { return IsValid(); }
	};

	class StringTableBuilder final
	{
	  public:
		StringTableEntry Add(std::string_view value)
		{
			if (value.empty())
			{
				return {};
			}

			const std::uint32_t offset = static_cast<std::uint32_t>(m_bytes.size());
			m_bytes.insert(m_bytes.end(), value.begin(), value.end());
			return StringTableEntry{offset, static_cast<std::uint32_t>(value.size())};
		}

		const std::vector<std::uint8_t>& GetBytes() const noexcept { return m_bytes; }
		std::vector<std::uint8_t>& GetBytes() noexcept { return m_bytes; }
		std::uint32_t SizeInBytes() const noexcept { return static_cast<std::uint32_t>(m_bytes.size()); }

	  private:
		std::vector<std::uint8_t> m_bytes;
	};
}