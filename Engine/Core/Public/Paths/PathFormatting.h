#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace PathFormatting
{
	std::string TimestampForFileName();
	std::filesystem::path TimestampedFileName(std::string_view stem, std::string_view extension);
	bool IsAsciiAlphaNumeric(char character) noexcept;
	char ToLowerAscii(char character) noexcept;
	std::string SanitizePathSegment(std::string_view segment, std::string_view fallback = "Unknown");
	bool EndsWithIgnoreCase(std::string_view value, std::string_view suffix) noexcept;
}
