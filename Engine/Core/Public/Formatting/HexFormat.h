#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Formatting
{
	SPARKLE_CORE_API std::string FormatHexUInt32(std::uint32_t value);
	SPARKLE_CORE_API std::string FormatPrefixedHexUInt32(std::uint32_t value);
	SPARKLE_CORE_API std::string FormatHexUInt64(std::uint64_t value);
	SPARKLE_CORE_API std::string FormatPrefixedHexUInt64(std::uint64_t value);
	SPARKLE_CORE_API bool TryParseHexUInt64(std::string_view text, std::uint64_t& outValue) noexcept;
}
