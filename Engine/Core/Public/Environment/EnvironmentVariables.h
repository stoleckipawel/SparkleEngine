#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>
#include <string>

namespace Environment
{
	SPARKLE_CORE_API bool TryGetVariable(const char* name, std::string& outValue) noexcept;
	SPARKLE_CORE_API bool GetFlag(const char* name) noexcept;
	SPARKLE_CORE_API std::uint32_t GetUInt32(
	    const char* name,
	    std::uint32_t fallbackValue) noexcept;
}  // namespace Environment
