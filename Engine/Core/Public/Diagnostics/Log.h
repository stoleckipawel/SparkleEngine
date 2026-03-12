// ============================================================================
// Log.h
// Public logging API for engine and client code.
// ----------------------------------------------------------------------------
#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>
#include <string_view>

// =============================================================================
// Log Levels
// =============================================================================

enum class LogLevel : std::uint8_t
{
	Trace = 0,    // Extremely verbose diagnostics: frame-by-frame, hot-path traces. High volume; usually disabled in release builds.
	Debug = 1,    // Developer-focused information for diagnosing control flow and intermediate state.
	Info = 2,     // High-level runtime events (startup, shutdown, subsystem init). Non-noisy normal operation logs.
	Warning = 3,  // Unexpected but recoverable conditions that may require attention (fallbacks, missing optional resources).
	Error = 4,    // Failures that prevent an operation from completing correctly but where the process may continue in degraded mode.
	Fatal = 5     // Unrecoverable errors: log synchronously, flush, break to debugger when attached, then terminate.
};

// =============================================================================
// Runtime Control (dynamic log level)
// =============================================================================

namespace Logger
{
	inline void SetLevel(LogLevel level) noexcept;
	inline LogLevel GetLevel() noexcept;
	inline bool IsEnabled(LogLevel level) noexcept;
}  // namespace Logger

// =============================================================================
// Compile-Time Filtering
//
// Reduce binary size and remove logging call-sites entirely at compile-time
// by setting `LE_COMPILE_LOG_LEVEL`. This is a simple and fast mechanism to
// avoid any runtime overhead for low-level traces in release builds.
// =============================================================================

#define LE_LOG_LEVEL_TRACE 0
#define LE_LOG_LEVEL_DEBUG 1
#define LE_LOG_LEVEL_INFO 2
#define LE_LOG_LEVEL_WARNING 3
#define LE_LOG_LEVEL_ERROR 4
#define LE_LOG_LEVEL_FATAL 5

#ifndef LE_COMPILE_LOG_LEVEL
	#ifdef NDEBUG
		#define LE_COMPILE_LOG_LEVEL LE_LOG_LEVEL_INFO
	#else
		#define LE_COMPILE_LOG_LEVEL LE_LOG_LEVEL_TRACE
	#endif
#endif

// =============================================================================
// Implementation Entry Points (internal)
// - `LogWrite` writes `msg` at the given `lvl` and prefixes the message with
//    the compact `file:line` location when available. It performs a fast
//    runtime-level check and emits the message in a single write.
// - `CheckHR` handles failed HRESULT-like values: it logs a fatal message,
//    flushes output, optionally breaks into the debugger, and terminates.
// =============================================================================

SPARKLE_CORE_API void LogWrite(std::string_view msg, LogLevel lvl, const char* file, std::uint32_t line) noexcept;
[[noreturn]] SPARKLE_CORE_API void CheckHR(long hr, const char* file, std::uint32_t line) noexcept;

// =============================================================================
// Logging Macros
//
// Simple, readable macros that capture source location automatically. Prefer
// these at call-sites to keep logging statements concise and consistent.
// =============================================================================

#define LE_LOG(lvl, msg) ::LogWrite((msg), (lvl), __FILE__, __LINE__)

#if LE_COMPILE_LOG_LEVEL <= LE_LOG_LEVEL_TRACE
	#define LOG_TRACE(msg) LE_LOG(LogLevel::Trace, msg)
#else
	#define LOG_TRACE(msg) ((void) 0)
#endif

#if LE_COMPILE_LOG_LEVEL <= LE_LOG_LEVEL_DEBUG
	#define LOG_DEBUG(msg) LE_LOG(LogLevel::Debug, msg)
#else
	#define LOG_DEBUG(msg) ((void) 0)
#endif

#if LE_COMPILE_LOG_LEVEL <= LE_LOG_LEVEL_INFO
	#define LOG_INFO(msg) LE_LOG(LogLevel::Info, msg)
#else
	#define LOG_INFO(msg) ((void) 0)
#endif

#if LE_COMPILE_LOG_LEVEL <= LE_LOG_LEVEL_WARNING
	#define LOG_WARNING(msg) LE_LOG(LogLevel::Warning, msg)
#else
	#define LOG_WARNING(msg) ((void) 0)
#endif

#if LE_COMPILE_LOG_LEVEL <= LE_LOG_LEVEL_ERROR
	#define LOG_ERROR(msg) LE_LOG(LogLevel::Error, msg)
#else
	#define LOG_ERROR(msg) ((void) 0)
#endif

#if LE_COMPILE_LOG_LEVEL <= LE_LOG_LEVEL_FATAL
	#define LOG_FATAL(msg) LE_LOG(LogLevel::Fatal, msg)
#else
	#define LOG_FATAL(msg) ((void) 0)
#endif

// =============================================================================
// HRESULT Validation
//
// Lightweight helper for functions that return HRESULT-like status. On
// failure it forwards to the implementation which prints a fatal message and
// terminates. Use in places where failure is unrecoverable during initialization
// or when continuing would corrupt program state.
// =============================================================================

#define CHECK(hr)                               \
	do                                          \
	{                                           \
		const long _hr = (hr);                  \
		if (_hr < 0)                            \
			::CheckHR(_hr, __FILE__, __LINE__); \
	} while (0)