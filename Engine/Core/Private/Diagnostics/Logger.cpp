#include "PCH.h"
#include "Logger.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <atomic>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#endif

namespace Logging
{
	static constexpr std::string_view kCoreLoggerName = "SparkleCore";

	using LoggerMap = std::unordered_map<std::string, std::shared_ptr<spdlog::logger>>;

	std::mutex& GetRegistryMutex() noexcept
	{
		static std::mutex registryMutex;
		return registryMutex;
	}

	LoggerMap& GetNamedLoggers() noexcept
	{
		static LoggerMap namedLoggers;
		return namedLoggers;
	}

	std::vector<spdlog::sink_ptr>& GetSharedSinks() noexcept
	{
		static std::vector<spdlog::sink_ptr> sharedSinks;
		return sharedSinks;
	}

	std::atomic<int>& GetLevelStorage() noexcept
	{
		static std::atomic<int> level{static_cast<int>(spdlog::level::info)};
		return level;
	}

	std::atomic<bool>& GetInitializedFlag() noexcept
	{
		static std::atomic<bool> initialized{false};
		return initialized;
	}

	std::optional<std::filesystem::path>& GetActiveLogFilePathStorage() noexcept
	{
		static std::optional<std::filesystem::path> activeLogFilePath;
		return activeLogFilePath;
	}

	spdlog::level::level_enum ReadConfiguredSpdlogLevel() noexcept
	{
		std::string configuredLevel;
		if (!Environment::TryGetVariable("SPARKLE_LOG_LEVEL", configuredLevel))
		{
			return spdlog::level::info;
		}

		for (char& character : configuredLevel)
		{
			if (character >= 'A' && character <= 'Z')
			{
				character = static_cast<char>(character - 'A' + 'a');
			}
		}

		const spdlog::level::level_enum configuredSpdlogLevel = spdlog::level::from_str(configuredLevel);
		return configuredSpdlogLevel == spdlog::level::off && configuredLevel != "off" ? spdlog::level::info : configuredSpdlogLevel;
	}

#if defined(_WIN32)
	class DebugOutputSink final : public spdlog::sinks::base_sink<std::mutex>
	{
	  protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
			formatted.push_back('\0');
			::OutputDebugStringA(formatted.data());
		}

		void flush_() override {}
	};
#endif

	std::vector<spdlog::sink_ptr> CreateDefaultSinks()
	{
		std::vector<spdlog::sink_ptr> sinks;
		GetActiveLogFilePathStorage().reset();

		auto stderrSink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
		stderrSink->set_pattern("[%n] [%l] %s:%# %v");
		sinks.push_back(stderrSink);

#if defined(_WIN32)
		auto debuggerSink = std::make_shared<DebugOutputSink>();
		debuggerSink->set_pattern("[%n] [%l] %s:%# %v");
		sinks.push_back(debuggerSink);
#endif

		try
		{
			std::string configuredFile;
			Environment::TryGetVariable("SPARKLE_LOG_FILE", configuredFile);
			const std::filesystem::path logPath = Paths::LogFile(configuredFile);

			auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
			fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %s:%# %v");
			sinks.push_back(fileSink);
			GetActiveLogFilePathStorage() = logPath;
		}
		catch (...)
		{
			return sinks;
		}

		return sinks;
	}

	void ApplyLoggerSettings(const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		if (!logger)
		{
			return;
		}

		logger->set_level(static_cast<spdlog::level::level_enum>(GetLevelStorage().load(std::memory_order_relaxed)));
		logger->flush_on(spdlog::level::err);
	}

	std::shared_ptr<spdlog::logger> CreateLogger(std::string_view name)
	{
		auto logger = std::make_shared<spdlog::logger>(std::string(name), GetSharedSinks().begin(), GetSharedSinks().end());
		ApplyLoggerSettings(logger);
		return logger;
	}

	void EnsureInitializedLocked()
	{
		if (GetInitializedFlag().load(std::memory_order_acquire))
		{
			return;
		}

		GetSharedSinks() = CreateDefaultSinks();
		GetLevelStorage().store(static_cast<int>(ReadConfiguredSpdlogLevel()), std::memory_order_relaxed);
		auto coreLogger = CreateLogger(kCoreLoggerName);
		GetNamedLoggers().emplace(std::string(kCoreLoggerName), coreLogger);
		spdlog::set_default_logger(coreLogger);
		GetInitializedFlag().store(true, std::memory_order_release);
	}

	void Initialize() noexcept
	{
		try
		{
			std::lock_guard<std::mutex> lock(GetRegistryMutex());
			EnsureInitializedLocked();
		}
		catch (...)
		{
			return;
		}
	}

	bool IsInitialized() noexcept
	{
		return GetInitializedFlag().load(std::memory_order_acquire);
	}

	std::optional<std::filesystem::path> GetActiveLogFilePath() noexcept
	{
		Initialize();

		try
		{
			std::lock_guard<std::mutex> lock(GetRegistryMutex());
			return GetActiveLogFilePathStorage();
		}
		catch (...)
		{
			return std::nullopt;
		}
	}

	std::shared_ptr<spdlog::logger> GetCoreLogger() noexcept
	{
		return GetOrCreateLogger(kCoreLoggerName);
	}

	std::shared_ptr<spdlog::logger> GetLogger(std::string_view name) noexcept
	{
		Initialize();

		std::lock_guard<std::mutex> lock(GetRegistryMutex());
		auto& namedLoggers = GetNamedLoggers();
		const auto it = namedLoggers.find(std::string(name));
		return it != namedLoggers.end() ? it->second : nullptr;
	}

	std::shared_ptr<spdlog::logger> GetOrCreateLogger(std::string_view name) noexcept
	{
		Initialize();

		{
			std::lock_guard<std::mutex> lock(GetRegistryMutex());
			auto& namedLoggers = GetNamedLoggers();
			const auto it = namedLoggers.find(std::string(name));
			if (it != namedLoggers.end())
			{
				return it->second;
			}
		}

		std::shared_ptr<spdlog::logger> logger;
		try
		{
			logger = CreateLogger(name);
		}
		catch (...)
		{
			logger = spdlog::default_logger();
		}

		std::lock_guard<std::mutex> lock(GetRegistryMutex());
		auto& namedLoggers = GetNamedLoggers();
		const auto [it, inserted] = namedLoggers.emplace(std::string(name), logger);
		if (!inserted)
		{
			return it->second;
		}

		if (logger)
		{
			return logger;
		}

		const auto coreLoggerIt = namedLoggers.find(std::string(kCoreLoggerName));
		if (coreLoggerIt != namedLoggers.end())
		{
			return coreLoggerIt->second;
		}

		return nullptr;
	}

	void SetLevel(spdlog::level::level_enum level) noexcept
	{
		GetLevelStorage().store(static_cast<int>(level), std::memory_order_relaxed);

		if (!IsInitialized())
		{
			return;
		}

		std::vector<std::shared_ptr<spdlog::logger>> loggers;
		{
			std::lock_guard<std::mutex> lock(GetRegistryMutex());
			loggers.reserve(GetNamedLoggers().size());
			for (const auto& [name, logger] : GetNamedLoggers())
			{
				(void) name;
				if (logger)
				{
					loggers.push_back(logger);
				}
			}
		}

		for (const std::shared_ptr<spdlog::logger>& logger : loggers)
		{
			logger->set_level(level);
		}
	}

	spdlog::level::level_enum GetLevel() noexcept
	{
		return static_cast<spdlog::level::level_enum>(GetLevelStorage().load(std::memory_order_relaxed));
	}
}
