#pragma once

#include "Core/Public/Diagnostics/Log.h"

#include <string_view>

namespace Logging
{
	inline std::string_view ExtractFileName(const char* path) noexcept
	{
		if (!path)
		{
			return {};
		}

		std::string_view text(path);
		for (std::size_t index = text.size(); index > 0; --index)
		{
			if (text[index - 1] == '/' || text[index - 1] == '\\')
			{
				return text.substr(index);
			}
		}

		return text;
	}

	inline const char* LevelTag(LogLevel level) noexcept
	{
		switch (level)
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
}