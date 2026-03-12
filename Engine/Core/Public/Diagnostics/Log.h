#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>
#include <string_view>

enum class LogLevel : std::uint8_t
{
	Trace = 0,
	Debug = 1,
	Info = 2,
	Warning = 3,
	Error = 4,
	Fatal = 5
};

namespace Logger
{
	inline void SetLevel(LogLevel level) noexcept;
	inline LogLevel GetLevel() noexcept;
	inline bool IsEnabled(LogLevel level) noexcept;
}

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

SPARKLE_CORE_API void LogWrite(std::string_view msg, LogLevel lvl, const char* file, std::uint32_t line) noexcept;
[[noreturn]] SPARKLE_CORE_API void CheckHR(long hr, const char* file, std::uint32_t line) noexcept;

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

#define CHECK(hr)                               \
	do                                          \
	{                                           \
		const long _hr = (hr);                  \
		if (_hr < 0)                            \
			::CheckHR(_hr, __FILE__, __LINE__); \
	} while (0)