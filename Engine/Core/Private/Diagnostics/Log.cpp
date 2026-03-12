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

	class Buffer
	{
	  public:
		void Append(const char* data, std::size_t len) noexcept
		{
			std::size_t n = (std::min) (len, kCapacity - m_pos - 1);
			std::memcpy(m_data + m_pos, data, n);
			m_pos += n;
		}

		void Append(std::string_view sv) noexcept { Append(sv.data(), sv.size()); }
		void Append(const char* s) noexcept
		{
			if (s)
				Append(s, std::strlen(s));
		}

		template <typename... Args> void Format(const char* fmt, Args... args) noexcept
		{
			std::size_t space = kCapacity - m_pos - 1;
			int n = std::snprintf(m_data + m_pos, space, fmt, args...);
			if (n > 0)
				m_pos += (std::min) (static_cast<std::size_t>(n), space);
		}

		void Newline() noexcept
		{
			if (m_pos < kCapacity)
				m_data[m_pos++] = '\n';
		}
		void Flush() noexcept
		{
			std::fwrite(m_data, 1, m_pos, stderr);

#if defined(_WIN32)
			::OutputDebugStringA(m_data);
#endif
		}

	  private:
		static constexpr std::size_t kCapacity = 2048;
		char m_data[kCapacity]{};
		std::size_t m_pos = 0;
	};
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

	Buffer buf;

	if (file)
	{
		buf.Append(ExtractFileName(file));
		buf.Format(":%u: ", static_cast<unsigned>(line));
	}

	buf.Append(LevelTag(lvl));
	buf.Append(msg);
	buf.Newline();
	buf.Flush();

	if (lvl == LogLevel::Fatal)
	{
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
