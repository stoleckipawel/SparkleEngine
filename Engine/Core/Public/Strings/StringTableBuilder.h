#pragma once

#include "Core/Public/CoreAPI.h"

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

	class SPARKLE_CORE_API StringTableBuilder final
	{
	  public:
		StringTableEntry Add(std::string_view value);

		const std::vector<std::uint8_t>& GetBytes() const noexcept { return m_bytes; }
		std::vector<std::uint8_t>& GetBytes() noexcept { return m_bytes; }
		std::uint32_t SizeInBytes() const noexcept { return static_cast<std::uint32_t>(m_bytes.size()); }

	  private:
		std::vector<std::uint8_t> m_bytes;
	};
}
