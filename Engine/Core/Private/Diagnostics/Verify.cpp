#include "PCH.h"
#include "Verify.h"

#include "Logger.h"
#include "Core/Public/Paths/PathUtils.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#endif

namespace Diagnostics
{
	void WriteFallback(std::string_view record) noexcept;

	std::string TrimTrailingWhitespace(std::string_view text)
	{
		while (!text.empty())
		{
			const char character = text.back();
			if (character != '\r' && character != '\n' && character != ' ' && character != '\t')
			{
				break;
			}
			text.remove_suffix(1);
		}

		return std::string(text);
	}

	std::string GetPlatformErrorSuffix(long hr) noexcept
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

		std::string suffix;
		if (length != 0 && systemMessage)
		{
			suffix = ": ";
			suffix += TrimTrailingWhitespace(std::string_view(systemMessage, length));
		}

		if (systemMessage)
		{
			::LocalFree(systemMessage);
		}

		return suffix;
#else
		(void) hr;
		return {};
#endif
	}

	std::string BuildMessagePrefix(const char* file, std::uint32_t line)
	{
		const std::string_view fileName = Paths::GetFileName(file ? file : "");
		std::string prefix;
		if (!fileName.empty())
		{
			prefix.append(fileName.data(), fileName.size());
			prefix.push_back(':');
			prefix.append(std::to_string(line));
			prefix.append(": ");
		}

		return prefix;
	}

	void WriteRecord(
	    const std::shared_ptr<spdlog::logger>& logger,
	    const char* file,
	    std::uint32_t line,
	    std::string_view message,
	    spdlog::level::level_enum level) noexcept
	{
		const auto writeFallback = [&]()
		{
			std::string fallbackRecord = BuildMessagePrefix(file, line);
			fallbackRecord.append(message.data(), message.size());
			WriteFallback(fallbackRecord);
		};

		if (logger)
		{
			try
			{
				const spdlog::source_loc location{file ? file : "", static_cast<int>(line), ""};
				logger->log(location, level, message);
				if (level >= spdlog::level::err)
				{
					logger->flush();
				}
				return;
			}
			catch (...)
			{
				writeFallback();
				return;
			}
		}

		writeFallback();
	}

	void BreakAttachedDebugger() noexcept
	{
#if defined(_WIN32) && !defined(NDEBUG)
		if (::IsDebuggerPresent())
		{
			::DebugBreak();
		}
#endif
	}

	void WriteFallback(std::string_view record) noexcept
	{
		std::fwrite(record.data(), 1, record.size(), stderr);
		std::fputc('\n', stderr);
		std::fflush(stderr);

#if defined(_WIN32)
		std::string debuggerRecord(record);
		debuggerRecord.push_back('\n');
		::OutputDebugStringA(debuggerRecord.c_str());
#endif
	}

	std::string BuildHResultRecord(long hr, const char* expression)
	{
		std::string record;
		record.reserve(96 + (expression ? std::char_traits<char>::length(expression) : 0));

		record.append("HRESULT 0x");

		char codeBuffer[9]{};
		std::snprintf(codeBuffer, sizeof(codeBuffer), "%08lX", static_cast<unsigned long>(hr));
		record.append(codeBuffer);

		if (expression && expression[0] != '\0')
		{
			record.append(" from ");
			record.append(expression);
		}

		record.append(GetPlatformErrorSuffix(hr));
		return record;
	}

	[[noreturn]] void Fatal(
	    const std::shared_ptr<spdlog::logger>& logger,
	    const char* file,
	    std::uint32_t line,
	    std::string_view message) noexcept
	{
		WriteRecord(logger, file, line, message, spdlog::level::critical);
		BreakAttachedDebugger();
		std::abort();
	}

	void BreakInDebuggerIfAttached() noexcept
	{
		BreakAttachedDebugger();
	}

	[[noreturn]] void CheckHResult(long hr, const char* expression, const char* file, std::uint32_t line) noexcept
	{
		const std::string record = BuildHResultRecord(hr, expression);
		auto logger = Logging::GetOrCreateLogger("Verify");
		WriteRecord(logger, file, line, record, spdlog::level::critical);
		BreakAttachedDebugger();
		std::abort();
	}
}
