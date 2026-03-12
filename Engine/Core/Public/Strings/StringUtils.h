// ============================================================================
// StringUtils.h
// ----------------------------------------------------------------------------
// String manipulation utilities for the engine.
//
// FUNCTIONS:
//   TrimAsciiWhitespace() - Trims whitespace from string ends
//   Unquote() - Removes surrounding double quotes
//   ToWide() - Converts narrow to wide string (ASCII)
//   ToWide(path) - Converts filesystem path to wide string
//
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Engine
{
	namespace Strings
	{
		// Trims ASCII whitespace (space, tab, CR, LF) from both ends of a string.
		constexpr std::string_view TrimAsciiWhitespace(std::string_view str) noexcept
		{
			constexpr std::string_view kWhitespace = " \t\r\n";
			const auto start = str.find_first_not_of(kWhitespace);
			if (start == std::string_view::npos)
			{
				return {};
			}
			const auto end = str.find_last_not_of(kWhitespace);
			return str.substr(start, end - start + 1);
		}

		// Removes surrounding double quotes from a string if present.
		constexpr std::string_view Unquote(std::string_view str) noexcept
		{
			if (str.size() >= 2 && str.front() == '"' && str.back() == '"')
			{
				return str.substr(1, str.size() - 2);
			}
			return str;
		}

		// Converts a narrow string to wide string (ASCII range only).
		inline std::wstring ToWide(std::string_view str)
		{
			return std::wstring(str.begin(), str.end());
		}

		// Converts a filesystem path to wide string.
		inline std::wstring ToWide(const std::filesystem::path& path)
		{
			return path.wstring();
		}

		// Converts a wide string to narrow string (ASCII range only).
		inline std::string ToNarrow(std::wstring_view str)
		{
			return std::string(str.begin(), str.end());
		}
	}  // namespace Strings
}  // namespace Engine
