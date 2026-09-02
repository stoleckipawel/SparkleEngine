#pragma once

#include "Core/Public/CoreAPI.h"
#include "Logger.h"

#include <cstdint>
#include <string_view>

namespace Diagnostics
{
	[[noreturn]] SPARKLE_CORE_API void Fatal(
	    const std::shared_ptr<spdlog::logger>& logger,
	    const char* file,
	    std::uint32_t line,
	    std::string_view message) noexcept;
	SPARKLE_CORE_API void BreakInDebuggerIfAttached() noexcept;
	[[noreturn]] SPARKLE_CORE_API void CheckHResult(
	    std::int32_t result,
	    const char* expression,
	    const char* file,
	    std::uint32_t line) noexcept;
}

#define CHECK(hr)                                                          \
	do                                                                     \
	{                                                                      \
		const std::int32_t _result = static_cast<std::int32_t>(hr);        \
		if (_result < 0)                                                   \
			::Diagnostics::CheckHResult(_result, #hr, __FILE__, __LINE__); \
	} while (0)
