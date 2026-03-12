#include "PCH.h"
#include "Log.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#endif

namespace
{
	// Runtime verbosity stored as an integer for fast, lock-free checks.
	// Default level is `Info` so normal engine messages are visible.
	std::atomic<int> g_level{static_cast<int>(LogLevel::Info)};

	std::string_view ExtractFileName(const char* path) noexcept
	{
		if (!path)
			return {};
		std::string_view sv(path);
		for (std::size_t i = sv.size(); i > 0; --i)
		{
			if (sv[i - 1] == '/' || sv[i - 1] == '\\')
				return sv.substr(i);
		}
		return sv;
	}

	// Fixed-width level tag used to make logs easy to scan.
	const char* LevelTag(LogLevel lvl) noexcept
	{
		switch (lvl)
		{
			case LogLevel::Trace:
				return "[TRACE]   ";
			case LogLevel::Debug:
				return "[DEBUG]   ";
			case LogLevel::Info:
				return "[INFO]    ";
			case LogLevel::Warning:
				return "[WARNING] ";
			case LogLevel::Error:
				return "[ERROR]   ";
			case LogLevel::Fatal:
				return "[FATAL]   ";
		}
		return "[?]       ";
	}

	void DebugBreakIfAttached() noexcept
	{
#if defined(_WIN32) && !defined(NDEBUG)
		if (::IsDebuggerPresent())
			::DebugBreak();
#endif
	}

	// Small stack buffer used for composing a single log message. The fixed
	// capacity keeps all hot-path operations allocation-free and avoids heap
	// fragmentation during heavy logging bursts.
	class Buffer
	{
	  public:
		// Append raw bytes into the buffer up to the remaining capacity.
		// Copies at most the available space minus one byte reserved for
		// a terminal newline or similar sentinel. This keeps the hot path
		// free of branches that would otherwise allocate.
		void Append(const char* data, std::size_t len) noexcept
		{
			std::size_t n = (std::min) (len, kCapacity - m_pos - 1);
			std::memcpy(m_data + m_pos, data, n);
			m_pos += n;
		}

		// Convenience overloads that forward into the raw append.
		void Append(std::string_view sv) noexcept { Append(sv.data(), sv.size()); }
		void Append(const char* s) noexcept
		{
			if (s)
				Append(s, std::strlen(s));
		}

		// Append formatted data. Uses snprintf but always limits writes to the
		// remaining buffer space so it cannot overflow. The formatted result
		// may be truncated if it does not fit; truncation is acceptable for
		// log messages and preserves the no-allocation guarantee.
		template <typename... Args> void Format(const char* fmt, Args... args) noexcept
		{
			std::size_t space = kCapacity - m_pos - 1;
			int n = std::snprintf(m_data + m_pos, space, fmt, args...);
			if (n > 0)
				m_pos += (std::min) (static_cast<std::size_t>(n), space);
		}

		// Helpers that finalize the message. Newline appends a single '\n'
		// and Flush performs a single fwrite call to emit the composed bytes.
		void Newline() noexcept
		{
			if (m_pos < kCapacity)
				m_data[m_pos++] = '\n';
		}
		void Flush() noexcept
		{
			// Write to stderr (console) first
			std::fwrite(m_data, 1, m_pos, stderr);

			// Also emit to the debugger output on Windows so messages are visible
			// in Visual Studio's Output window when running under the debugger.
#if defined(_WIN32)
			::OutputDebugStringA(m_data);
#endif
		}

	  private:
		// Fixed stack capacity chosen to comfortably hold typical log lines
		// (file:line + level tag + message). Keeping this on the stack makes
		// logging cheap and avoids heap churn during bursts.
		static constexpr std::size_t kCapacity = 2048;  // tuned for typical messages
		char m_data[kCapacity]{};                       // zero-initialized for clarity when printed
		std::size_t m_pos = 0;                          // current write position
	};
}  // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

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
}  // namespace Logger

void LogWrite(std::string_view msg, LogLevel lvl, const char* file, std::uint32_t line) noexcept
{
	// Fast-path: level filtered out (lock-free check)
	if (static_cast<int>(lvl) < g_level.load(std::memory_order_relaxed))
	{
		return;
	}

	Buffer buf;

	// Prefix with compact file:line where available
	if (file)
	{
		buf.Append(ExtractFileName(file));
		buf.Format(":%u: ", static_cast<unsigned>(line));
	}

	// Append level tag + payload and write out in a single syscall
	buf.Append(LevelTag(lvl));
	buf.Append(msg);
	buf.Newline();
	buf.Flush();

	if (lvl == LogLevel::Fatal)
	{
		// Ensure output is observed, break to debugger if attached, then abort
		std::fflush(stderr);
		DebugBreakIfAttached();
		std::abort();
	}
}

[[noreturn]] void CheckHR(long hr, const char* file, std::uint32_t line) noexcept
{
	Buffer buf;
	if (file)
	{
		buf.Append(ExtractFileName(file));
		buf.Format(":%u: ", static_cast<unsigned>(line));
	}
	buf.Append(LevelTag(LogLevel::Fatal));
	buf.Format("HRESULT 0x%08lX", static_cast<unsigned long>(hr));
	buf.Newline();
	buf.Flush();
	std::fflush(stderr);

	DebugBreakIfAttached();
	std::abort();
}
