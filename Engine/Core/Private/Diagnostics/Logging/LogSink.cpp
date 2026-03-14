#include "PCH.h"
#include "Diagnostics/Logging/LogSink.h"

#include "Diagnostics/Logging/LogBuffer.h"

#include <cstdio>
#include <mutex>
#include <string_view>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#endif

namespace
{
	std::mutex g_sinkMutex;

#if defined(_WIN32)
	std::string_view TrimTrailingWhitespace(std::string_view text) noexcept
	{
		while (!text.empty())
		{
			const char c = text.back();
			if (c != '\r' && c != '\n' && c != ' ' && c != '\t')
			{
				break;
			}
			text.remove_suffix(1);
		}

		return text;
	}
#endif
}

namespace Logging
{
	void WriteToSinks(Buffer& buffer) noexcept
	{
		std::lock_guard<std::mutex> lock(g_sinkMutex);
		std::fwrite(buffer.Data(), 1, buffer.Size(), stderr);

#if defined(_WIN32)
		::OutputDebugStringA(buffer.CStr());
#endif
	}

	void AppendPlatformError(Buffer& buffer, long hr) noexcept
	{
#if defined(_WIN32)
		char* systemMessage = nullptr;
		const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
		const DWORD length = ::FormatMessageA(
		    flags,
		    nullptr,
		    static_cast<DWORD>(hr),
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    reinterpret_cast<LPSTR>(&systemMessage),
		    0,
		    nullptr);

		if (length != 0 && systemMessage)
		{
			buffer.Append(": ");
			buffer.Append(TrimTrailingWhitespace(std::string_view(systemMessage, length)));
		}

		if (systemMessage)
		{
			::LocalFree(systemMessage);
		}
#else
		(void) buffer;
		(void) hr;
#endif
	}

	void BreakInDebuggerIfAttached() noexcept
	{
#if defined(_WIN32) && !defined(NDEBUG)
		if (::IsDebuggerPresent())
		{
			::DebugBreak();
		}
#endif
	}
}