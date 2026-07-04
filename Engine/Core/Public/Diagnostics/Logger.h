#pragma once

#include "Core/Public/CoreAPI.h"

#include <memory>
#include <string_view>

#include <spdlog/spdlog.h>

namespace Logging
{
	// Acquire a named engine-owned logger and log through SPDLOG_LOGGER_* macros.
	// SparkleCore owns bootstrap, sink policy, and logger lifetime.
	SPARKLE_CORE_API void Initialize() noexcept;
	SPARKLE_CORE_API bool IsInitialized() noexcept;
	SPARKLE_CORE_API std::shared_ptr<spdlog::logger> GetCoreLogger() noexcept;
	SPARKLE_CORE_API std::shared_ptr<spdlog::logger> GetLogger(std::string_view name) noexcept;
	SPARKLE_CORE_API std::shared_ptr<spdlog::logger> GetOrCreateLogger(std::string_view name) noexcept;
	SPARKLE_CORE_API void SetLevel(spdlog::level::level_enum level) noexcept;
	SPARKLE_CORE_API spdlog::level::level_enum GetLevel() noexcept;
}
