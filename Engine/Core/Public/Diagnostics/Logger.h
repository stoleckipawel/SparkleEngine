#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

namespace Engine::Logging
{
	struct LogRecord final
	{
		std::string LoggerName;
		spdlog::level::level_enum Level = spdlog::level::info;
		std::string Message;
	};

	using LogRecordHandler = std::function<void(LogRecord)>;

	// Acquire a named engine-owned logger and log through SPDLOG_LOGGER_* macros.
	// SparkleCore owns bootstrap, sink policy, and logger lifetime.
	SPARKLE_CORE_API void Initialize() noexcept;
	SPARKLE_CORE_API bool IsInitialized() noexcept;
	SPARKLE_CORE_API std::shared_ptr<spdlog::logger> GetCoreLogger() noexcept;
	SPARKLE_CORE_API std::shared_ptr<spdlog::logger> GetLogger(std::string_view name) noexcept;
	SPARKLE_CORE_API std::shared_ptr<spdlog::logger> GetOrCreateLogger(std::string_view name) noexcept;
	SPARKLE_CORE_API std::uint64_t AddRecordHandler(LogRecordHandler handler) noexcept;
	SPARKLE_CORE_API void RemoveRecordHandler(std::uint64_t handlerId) noexcept;
	SPARKLE_CORE_API void SetLevel(spdlog::level::level_enum level) noexcept;
	SPARKLE_CORE_API spdlog::level::level_enum GetLevel() noexcept;
}