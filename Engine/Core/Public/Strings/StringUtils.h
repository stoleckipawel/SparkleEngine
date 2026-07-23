#pragma once

#include "Core/Public/CoreAPI.h"

#include <DirectXMath.h>

#include <charconv>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

namespace Strings
{
	SPARKLE_CORE_API bool IsAsciiWhitespace(char character) noexcept;
	SPARKLE_CORE_API bool ContainsAsciiWhitespace(std::string_view str) noexcept;
	SPARKLE_CORE_API std::string_view TrimAsciiWhitespace(std::string_view str) noexcept;
	SPARKLE_CORE_API std::string_view Unquote(std::string_view str) noexcept;
	SPARKLE_CORE_API std::string TrimCopy(std::string_view str);
	SPARKLE_CORE_API std::string UnquoteCopy(std::string_view str);
	SPARKLE_CORE_API std::vector<std::string_view> Split(
	    std::string_view str,
	    char separator,
	    bool keepEmptyFields = true);
	SPARKLE_CORE_API std::string ToLowerCopy(std::string_view str);
	SPARKLE_CORE_API std::string EscapeCsvField(std::string_view str);
	SPARKLE_CORE_API std::string EscapeJsonString(std::string_view str);
	SPARKLE_CORE_API bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept;
	SPARKLE_CORE_API bool StartsWithIgnoreCase(std::string_view value, std::string_view prefix) noexcept;
	SPARKLE_CORE_API bool ContainsIgnoreCase(std::string_view haystack, std::string_view needle) noexcept;
	SPARKLE_CORE_API bool TryParseBool(std::string_view str, bool& outValue);
	SPARKLE_CORE_API bool TrySplitKeyValue(
	    std::string_view str,
	    char separator,
	    std::string_view& outKey,
	    std::string_view& outValue) noexcept;
	SPARKLE_CORE_API std::string Join(
	    std::span<const std::string_view> values,
	    std::string_view separator,
	    std::size_t firstValueIndex = 0);

	template <typename TNumber> bool TryParseNumber(std::string_view str, TNumber& outValue)
	{
		static_assert(std::is_arithmetic_v<TNumber>);

		const std::string_view trimmed = TrimAsciiWhitespace(str);
		if (trimmed.empty())
		{
			return false;
		}

		const char* begin = trimmed.data();
		const char* end = trimmed.data() + trimmed.size();
		const std::from_chars_result result = std::from_chars(begin, end, outValue);
		return result.ec == std::errc {} && result.ptr == end;
	}

	SPARKLE_CORE_API bool TryParseFloat(std::string_view str, float& outValue);
	SPARKLE_CORE_API bool TryParseFloat3(std::string_view str, DirectX::XMFLOAT3& outValue);
	SPARKLE_CORE_API std::wstring ToWide(std::string_view str);
	SPARKLE_CORE_API std::wstring ToWide(const std::filesystem::path& path);
	SPARKLE_CORE_API std::string ToNarrow(std::wstring_view str);
}  // namespace Strings
