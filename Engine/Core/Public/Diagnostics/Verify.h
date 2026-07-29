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
	[[noreturn]] SPARKLE_CORE_API void CheckHResult(long hr, const char* expression, const char* file, std::uint32_t line) noexcept;
}

#define CHECK(hr)                                                      \
	do                                                                 \
	{                                                                  \
		const long _hr = static_cast<long>(hr);                        \
		if (_hr < 0)                                                   \
			::Diagnostics::CheckHResult(_hr, #hr, __FILE__, __LINE__); \
	} while (0)
