#pragma once

#include "Core/Public/Diagnostics/Log.h"
#include "Core/Public/Paths/PathUtils.h"

#include <string_view>

namespace Logging
{
	inline std::string_view ExtractFileName(const char* path) noexcept
	{
		if (!path)
		{
			return {};
		}

		return Engine::Paths::GetFileName(path);
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
}  // namespace Logging