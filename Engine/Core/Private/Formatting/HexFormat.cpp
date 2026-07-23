#include "PCH.h"

#include "Core/Public/Formatting/HexFormat.h"

#include "Core/Public/Strings/StringUtils.h"

#include <charconv>
#include <format>
#include <system_error>

namespace Formatting
{
	std::string FormatHexUInt32(std::uint32_t value)
	{
		return std::format("{:08X}", value);
	}

	std::string FormatPrefixedHexUInt32(std::uint32_t value)
	{
		return "0x" + FormatHexUInt32(value);
	}

	std::string FormatHexUInt64(std::uint64_t value)
	{
		return std::format("{:016X}", value);
	}

	std::string FormatPrefixedHexUInt64(std::uint64_t value)
	{
		return "0x" + FormatHexUInt64(value);
	}

	bool TryParseHexUInt64(std::string_view text, std::uint64_t& outValue) noexcept
	{
		std::string_view trimmed = Strings::TrimAsciiWhitespace(text);
		if (trimmed.starts_with("0x") || trimmed.starts_with("0X"))
		{
			trimmed.remove_prefix(2);
		}
		if (trimmed.empty())
		{
			return false;
		}

		const std::from_chars_result result =
		    std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), outValue, 16);
		return result.ec == std::errc {} && result.ptr == trimmed.data() + trimmed.size();
	}
}  // namespace Formatting
