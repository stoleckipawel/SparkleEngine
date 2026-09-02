#include "PCH.h"
#include "Verify.h"

#include "Logger.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/PathUtils.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <spdlog/common.h>
#include <spdlog/logger.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <windows.h>
  #include <debugapi.h>
  #include <minwindef.h>
  #include <WinBase.h>
  #include <winnt.h>
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

	std::string GetPlatformErrorSuffix(std::int32_t result)
	{
#ifdef _WIN32
		constexpr DWORD systemMessageCapacity = 512;
		std::array<char, systemMessageCapacity> systemMessage{};
		constexpr DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
		const DWORD length = ::FormatMessageA(
		    flags,
		    nullptr,
		    static_cast<DWORD>(result),
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    systemMessage.data(),
		    static_cast<DWORD>(systemMessage.size()),
		    nullptr);

		std::string suffix;
		if (length != 0)
		{
			suffix = ": ";
			suffix += TrimTrailingWhitespace(std::string_view(systemMessage.data(), length));
		}

		return suffix;
#else
		(void) result;
		return {};
#endif
	}

	std::string BuildMessagePrefix(const char* file, std::uint32_t line)
	{
		const std::string_view fileName = Paths::GetFileName(file != nullptr ? file : "");
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
		try
		{
			if (logger)
			{
				const spdlog::source_loc location{file != nullptr ? file : "", static_cast<int>(line), ""};
				logger->log(location, level, message);
				if (level >= spdlog::level::err)
				{
					logger->flush();
				}
				return;
			}
		}
		catch (...)
		{
			WriteFallback(message);
			return;
		}

		try
		{
			std::string fallbackRecord = BuildMessagePrefix(file, line);
			fallbackRecord.append(message.data(), message.size());
			WriteFallback(fallbackRecord);
		}
		catch (...)
		{
			WriteFallback(message);
		}
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
		(void) std::fwrite(record.data(), 1, record.size(), stderr);
		(void) std::fputc('\n', stderr);
		(void) std::fflush(stderr);

#ifdef _WIN32
		try
		{
			std::string debuggerRecord(record);
			debuggerRecord.push_back('\n');
			::OutputDebugStringA(debuggerRecord.c_str());
		}
		catch (...)
		{
			::OutputDebugStringA("Diagnostic record allocation failed.\n");
		}
#endif
	}

	std::string BuildHResultRecord(std::int32_t result, const char* expression)
	{
		std::string record = "HRESULT ";
		record.append(Formatting::FormatPrefixedHexUInt32(static_cast<std::uint32_t>(result)));

		if (expression != nullptr && *expression != '\0')
		{
			record.append(" from ");
			record.append(expression);
		}

		record.append(GetPlatformErrorSuffix(result));
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

	[[noreturn]] void CheckHResult(std::int32_t result, const char* expression, const char* file, std::uint32_t line) noexcept
	{
		try
		{
			const std::string record = BuildHResultRecord(result, expression);
			auto logger = Logging::GetOrCreateLogger("Verify");
			WriteRecord(logger, file, line, record, spdlog::level::critical);
		}
		catch (...)
		{
			WriteFallback("HRESULT verification failed while building its diagnostic record.");
		}
		BreakAttachedDebugger();
		std::abort();
	}
}
