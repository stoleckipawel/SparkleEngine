#pragma once

#include "Core/Public/CoreMacros.h"
#include "Core/Public/Diagnostics/Logger.h"

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

namespace Engine::Diagnostics
{
	class ScopedLogEvent final
	{
	  public:
		ScopedLogEvent(std::shared_ptr<spdlog::logger> logger, spdlog::level::level_enum level, std::string_view label) noexcept :
		    m_logger(std::move(logger)), m_level(level), m_label(label)
		{
			if (m_logger == nullptr || m_label.empty() || !m_logger->should_log(m_level))
			{
				return;
			}

			m_startTime = Clock::now();
			m_enabled = true;
			SPDLOG_LOGGER_CALL(m_logger.get(), m_level, "{} begin", m_label);
		}

		~ScopedLogEvent() noexcept
		{
			if (!m_enabled || m_logger == nullptr)
			{
				return;
			}

			const double elapsedMilliseconds =
			    std::chrono::duration<double, std::milli>(Clock::now() - m_startTime).count();
			SPDLOG_LOGGER_CALL(m_logger.get(), m_level, "{} end ({:.3f} ms)", m_label, elapsedMilliseconds);
		}

		ScopedLogEvent(const ScopedLogEvent&) = delete;
		ScopedLogEvent& operator=(const ScopedLogEvent&) = delete;
		ScopedLogEvent(ScopedLogEvent&&) = delete;
		ScopedLogEvent& operator=(ScopedLogEvent&&) = delete;

	  private:
		using Clock = std::chrono::steady_clock;

		std::shared_ptr<spdlog::logger> m_logger;
		spdlog::level::level_enum m_level = spdlog::level::info;
		std::string m_label;
		Clock::time_point m_startTime{};
		bool m_enabled = false;
	};
}

#ifndef SPARKLE_LOG_SCOPE
#define SPARKLE_LOG_SCOPE(logger, level, label) \
	::Engine::Diagnostics::ScopedLogEvent SPARKLE_PP_CONCAT(_sparkleScopedLogEvent_, __LINE__){(logger), (level), (label)}
#endif