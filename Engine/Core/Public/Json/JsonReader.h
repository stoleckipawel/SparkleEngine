#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstddef>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace Json
{
	SPARKLE_CORE_API std::size_t FindPropertyValue(std::string_view objectText, std::string_view key) noexcept;
	SPARKLE_CORE_API bool TryReadStringProperty(std::string_view objectText, std::string_view key, std::string& outValue);

	template <typename TNumber> bool TryReadUnsignedIntegerProperty(std::string_view objectText, std::string_view key, TNumber& outValue)
	{
		std::size_t cursor = FindPropertyValue(objectText, key);
		if (cursor == std::string_view::npos)
		{
			return false;
		}

		const std::size_t numberStart = cursor;
		while (cursor < objectText.size() && std::isdigit(static_cast<unsigned char>(objectText[cursor])))
		{
			++cursor;
		}
		if (cursor == numberStart)
		{
			return false;
		}

		const std::from_chars_result result = std::from_chars(objectText.data() + numberStart, objectText.data() + cursor, outValue);
		return result.ec == std::errc{} && result.ptr == objectText.data() + cursor;
	}

	SPARKLE_CORE_API bool TryReadUInt64Property(std::string_view objectText, std::string_view key, std::uint64_t& outValue);
	SPARKLE_CORE_API bool TryParseHexUInt64(std::string_view text, std::uint64_t& outValue) noexcept;
}
