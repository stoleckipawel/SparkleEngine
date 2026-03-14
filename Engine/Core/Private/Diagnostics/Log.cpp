#include "PCH.h"
#include "Log.h"

#include "Diagnostics/Logging/LogBuffer.h"
#include "Diagnostics/Logging/LogFormatting.h"
#include "Diagnostics/Logging/LogSink.h"

#include <atomic>
#include <cstdlib>

namespace
{
	std::atomic<int> g_level{static_cast<int>(LogLevel::Info)};
}

namespace Logger
{
	void SetLevel(LogLevel level) noexcept
	{
		g_level.store(static_cast<int>(level), std::memory_order_relaxed);
	}

	LogLevel GetLevel() noexcept
	{
		return static_cast<LogLevel>(g_level.load(std::memory_order_relaxed));
	}

	bool IsEnabled(LogLevel level) noexcept
	{
		return static_cast<int>(level) >= g_level.load(std::memory_order_relaxed);
	}
}

void LogWrite(std::string_view msg, LogLevel lvl, const char* file, std::uint32_t line) noexcept
{
	if (static_cast<int>(lvl) < g_level.load(std::memory_order_relaxed))
	{
		return;
	}

	Logging::Buffer buffer;

	if (file)
	{
		buffer.Append(Logging::ExtractFileName(file));
		buffer.Format(":%u: ", static_cast<unsigned>(line));
	}

	buffer.Append(Logging::LevelTag(lvl));
	buffer.Append(msg);
	buffer.Newline();
	Logging::WriteToSinks(buffer);

	if (lvl == LogLevel::Fatal)
	{
		std::fflush(stderr);
		Logging::BreakInDebuggerIfAttached();
		std::abort();
	}
}

[[noreturn]] void CheckHR(long hr, const char* file, std::uint32_t line) noexcept
{
	Logging::Buffer buffer;
	if (file)
	{
		buffer.Append(Logging::ExtractFileName(file));
		buffer.Format(":%u: ", static_cast<unsigned>(line));
	}
	buffer.Append(Logging::LevelTag(LogLevel::Fatal));
	buffer.Format("HRESULT 0x%08lX", static_cast<unsigned long>(hr));
	Logging::AppendPlatformError(buffer, hr);
	buffer.Newline();
	Logging::WriteToSinks(buffer);
	std::fflush(stderr);

	Logging::BreakInDebuggerIfAttached();
	std::abort();
}
