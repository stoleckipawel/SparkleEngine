#include "PCH.h"
#include "Logger.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <mutex>
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

	namespace Detail
	{
		using LoggerMap = std::unordered_map<std::string, std::shared_ptr<spdlog::logger>>;
		using LogRecordHandlerMap = std::unordered_map<std::uint64_t, LogRecordHandler>;

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

		std::mutex& GetLogRecordHandlerMutex() noexcept
		{
			static std::mutex handlerMutex;
			return handlerMutex;
		}

		LogRecordHandlerMap& GetLogRecordHandlers() noexcept
		{
			static LogRecordHandlerMap handlers;
			return handlers;
		}

		std::atomic<std::uint64_t>& GetNextLogRecordHandlerId() noexcept
		{
			static std::atomic<std::uint64_t> nextHandlerId{1};
			return nextHandlerId;
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

		void PublishLogRecord(LogRecord record) noexcept
		{
			std::vector<LogRecordHandler> handlers;
			{
				std::lock_guard<std::mutex> lock(GetLogRecordHandlerMutex());
				handlers.reserve(GetLogRecordHandlers().size());
				for (const auto& [id, handler] : GetLogRecordHandlers())
				{
					(void) id;
					if (handler)
					{
						handlers.push_back(handler);
					}
				}
			}

			for (const LogRecordHandler& handler : handlers)
			{
				try
				{
					handler(record);
				}
				catch (...)
				{
				}
			}
		}

		class LogObserverSink final : public spdlog::sinks::base_sink<std::mutex>
		{
		  protected:
			void sink_it_(const spdlog::details::log_msg& msg) override
			{
				PublishLogRecord(
				    LogRecord{
				        .LoggerName = std::string(msg.logger_name.data(), msg.logger_name.size()),
				        .Level = msg.level,
				        .Message = std::string(msg.payload.data(), msg.payload.size()),
				    });
			}

			void flush_() override {}
		};

		std::vector<spdlog::sink_ptr> CreateDefaultSinks()
		{
			std::vector<spdlog::sink_ptr> sinks;

			auto observerSink = std::make_shared<LogObserverSink>();
			sinks.push_back(observerSink);

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
			}
			catch (...)
			{
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
			auto coreLogger = CreateLogger(kCoreLoggerName);
			GetNamedLoggers().emplace(std::string(kCoreLoggerName), coreLogger);
			spdlog::set_default_logger(coreLogger);
			GetInitializedFlag().store(true, std::memory_order_release);
		}
	}  // namespace Detail

	void Initialize() noexcept
	{
		try
		{
			std::lock_guard<std::mutex> lock(Detail::GetRegistryMutex());
			Detail::EnsureInitializedLocked();
		}
		catch (...)
		{
		}
	}

	bool IsInitialized() noexcept
	{
		return Detail::GetInitializedFlag().load(std::memory_order_acquire);
	}

	std::shared_ptr<spdlog::logger> GetCoreLogger() noexcept
	{
		return GetOrCreateLogger(kCoreLoggerName);
	}

	std::shared_ptr<spdlog::logger> GetLogger(std::string_view name) noexcept
	{
		Initialize();

		std::lock_guard<std::mutex> lock(Detail::GetRegistryMutex());
		auto& namedLoggers = Detail::GetNamedLoggers();
		const auto it = namedLoggers.find(std::string(name));
		return it != namedLoggers.end() ? it->second : nullptr;
	}

	std::shared_ptr<spdlog::logger> GetOrCreateLogger(std::string_view name) noexcept
	{
		Initialize();

		std::lock_guard<std::mutex> lock(Detail::GetRegistryMutex());
		auto& namedLoggers = Detail::GetNamedLoggers();
		const auto it = namedLoggers.find(std::string(name));
		if (it != namedLoggers.end())
		{
			return it->second;
		}

		try
		{
			auto logger = Detail::CreateLogger(name);
			namedLoggers.emplace(std::string(name), logger);
			return logger;
		}
		catch (...)
		{
			const auto coreLoggerIt = namedLoggers.find(std::string(kCoreLoggerName));
			if (coreLoggerIt != namedLoggers.end())
			{
				return coreLoggerIt->second;
			}

			return spdlog::default_logger();
		}
	}

	std::uint64_t AddRecordHandler(LogRecordHandler handler) noexcept
	{
		if (!handler)
		{
			return 0;
		}

		try
		{
			Initialize();
			const std::uint64_t handlerId = Detail::GetNextLogRecordHandlerId().fetch_add(1, std::memory_order_relaxed);
			std::lock_guard<std::mutex> lock(Detail::GetLogRecordHandlerMutex());
			Detail::GetLogRecordHandlers().emplace(handlerId, std::move(handler));
			return handlerId;
		}
		catch (...)
		{
			return 0;
		}
	}

	void RemoveRecordHandler(std::uint64_t handlerId) noexcept
	{
		if (handlerId == 0)
		{
			return;
		}

		try
		{
			std::lock_guard<std::mutex> lock(Detail::GetLogRecordHandlerMutex());
			Detail::GetLogRecordHandlers().erase(handlerId);
		}
		catch (...)
		{
		}
	}

	void SetLevel(spdlog::level::level_enum level) noexcept
	{
		Detail::GetLevelStorage().store(static_cast<int>(level), std::memory_order_relaxed);

		if (!IsInitialized())
		{
			return;
		}

		std::lock_guard<std::mutex> lock(Detail::GetRegistryMutex());
		for (const auto& [name, logger] : Detail::GetNamedLoggers())
		{
			(void) name;
			if (logger)
			{
				logger->set_level(level);
			}
		}
	}

	spdlog::level::level_enum GetLevel() noexcept
	{
		return static_cast<spdlog::level::level_enum>(Detail::GetLevelStorage().load(std::memory_order_relaxed));
	}
}