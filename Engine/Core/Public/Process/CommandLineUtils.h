#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace CommandLine
{
	SPARKLE_CORE_API std::wstring_view ReadToken(std::wstring_view commandLine, std::size_t& offset) noexcept;
	SPARKLE_CORE_API std::string QuoteArgument(std::string_view text);
	SPARKLE_CORE_API std::string QuotePath(const std::filesystem::path& path);
}
