#include "PCH.h"

#include "Core/Public/Diagnostics/Trace.h"

#include "Core/Public/Diagnostics/LiveProfiler.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Time/Timer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <format>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Diagnostics
{
	class TraceService final
	{
	  public:
		struct TraceEventRecord final
		{
			std::uint64_t TimestampMicroseconds = 0;
			std::uint32_t ThreadId = 0;
			std::uint32_t NameId = 0;
			ETraceCategory Category = ETraceCategory::Default;
			char Phase = 'B';
		};

		struct TransparentStringHash final
		{
			using is_transparent = void;

			std::size_t operator()(std::string_view value) const noexcept { return std::hash<std::string_view>{}(value); }
			std::size_t operator()(const std::string& value) const noexcept { return (*this)(std::string_view(value)); }
			std::size_t operator()(const char* value) const noexcept { return (*this)(value != nullptr ? std::string_view(value) : std::string_view{}); }
		};

		struct TransparentStringEqual final
		{
			using is_transparent = void;

			bool operator()(std::string_view left, std::string_view right) const noexcept { return left == right; }
		};

		struct ThreadBuffer final
		{
			ThreadBuffer() noexcept : Owner(&TraceService::Get()), ThreadId(TraceService::GetCurrentThreadIdValue())
			{
				Events.reserve(TraceService::kThreadBufferCapacity);
				Owner->RegisterThreadBuffer(*this);
			}

			~ThreadBuffer() noexcept
			{
				if (Owner != nullptr)
				{
					Owner->UnregisterThreadBuffer(*this);
				}
			}

			TraceService* Owner = nullptr;
			std::uint32_t ThreadId = 0;
			std::mutex Mutex;
			std::vector<TraceEventRecord> Events;
		};

		static constexpr std::size_t kThreadBufferCapacity = 256;

		static TraceService& Get() noexcept
		{
			static TraceService* instance = new TraceService();
			return *instance;
		}

		void BeginSession(const TraceSessionConfig& config) noexcept
		{
			m_sessionActive.store(false, std::memory_order_release);
			FlushAllThreadBuffers();

			std::lock_guard<std::mutex> lock(m_stateMutex);
			m_events.clear();
			m_nameIds.clear();
			m_names.clear();
			m_names.push_back({});
			m_nextNameId = 1;
			m_sessionStart = Clock::now();
			m_outputPath = config.OutputPath.empty() ? std::filesystem::path{"logs/trace.json"} : Paths::Normalize(config.OutputPath);
			m_exportOnEnd = config.ExportOnEnd;
			m_logMirroringEnabled = config.EnableLogMirroring;
			m_logMirrorLogger = m_logMirroringEnabled ? Logging::GetOrCreateLogger("Diagnostics.Trace") : nullptr;
			m_sessionActive.store(true, std::memory_order_release);
		}

		void EndSession() noexcept
		{
			const bool shouldExport = m_exportOnEnd;
			FlushInternal(false, shouldExport);
		}

		void Flush() noexcept
		{
			FlushInternal(true, true);
		}

		bool IsSessionActive() const noexcept
		{
			return m_sessionActive.load(std::memory_order_acquire);
		}

		std::uint32_t BeginScope(
		    std::string_view name,
		    ETraceCategory category,
		    std::uint64_t& outBeginTimestampMicroseconds) noexcept
		{
			outBeginTimestampMicroseconds = 0;
			if (!IsSessionActive() || name.empty())
			{
				return 0;
			}

			const std::uint64_t timestamp = GetTimestampMicroseconds();
			const std::uint32_t nameId = InternName(name);
			AppendEvent(TraceEventRecord{
			    .TimestampMicroseconds = timestamp,
			    .ThreadId = GetCurrentThreadIdValue(),
			    .NameId = nameId,
			    .Category = category,
			    .Phase = 'B'});

			outBeginTimestampMicroseconds = timestamp;

			if (m_logMirroringEnabled)
			{
				const std::shared_ptr<spdlog::logger> logger = ResolveMirrorLogger();
				if (logger != nullptr && logger->should_log(spdlog::level::info))
				{
					SPDLOG_LOGGER_INFO(logger, "{} begin", name);
				}
			}

			return nameId;
		}

		void EndScope(
		    std::uint32_t nameId,
		    ETraceCategory category,
		    std::uint64_t beginTimestampMicroseconds) noexcept
		{
			if (!IsSessionActive() || nameId == 0)
			{
				return;
			}

			const std::uint64_t endTimestamp = GetTimestampMicroseconds();
			AppendEvent(TraceEventRecord{
			    .TimestampMicroseconds = endTimestamp,
			    .ThreadId = GetCurrentThreadIdValue(),
			    .NameId = nameId,
			    .Category = category,
			    .Phase = 'E'});

			if (m_logMirroringEnabled)
			{
				const std::shared_ptr<spdlog::logger> logger = ResolveMirrorLogger();
				if (logger != nullptr && logger->should_log(spdlog::level::info))
				{
					const std::string name = LookupName(nameId);
					const double elapsedMs =
					    static_cast<double>(endTimestamp - beginTimestampMicroseconds) / 1000.0;
					SPDLOG_LOGGER_INFO(logger, "{} end ({:.3f} ms)", name, elapsedMs);
				}
			}
		}

	  private:
		using NameIdMap = std::unordered_map<std::string, std::uint32_t, TransparentStringHash, TransparentStringEqual>;

		TraceService() noexcept
		{
			m_names.push_back({});
			m_sessionStart = Clock::now();
		}

		static std::uint32_t GetCurrentThreadIdValue() noexcept
		{
			return static_cast<std::uint32_t>(::GetCurrentThreadId());
		}

		static ThreadBuffer& GetThreadBuffer() noexcept
		{
			thread_local ThreadBuffer buffer;
			return buffer;
		}

		void RegisterThreadBuffer(ThreadBuffer& buffer) noexcept
		{
			std::lock_guard<std::mutex> lock(m_stateMutex);
			m_threadBuffers.push_back(&buffer);
		}

		void UnregisterThreadBuffer(ThreadBuffer& buffer) noexcept
		{
			FlushThreadBuffer(buffer);

			std::lock_guard<std::mutex> lock(m_stateMutex);
			auto removeIt = std::remove(m_threadBuffers.begin(), m_threadBuffers.end(), &buffer);
			m_threadBuffers.erase(removeIt, m_threadBuffers.end());
		}

		void FlushThreadBuffer(ThreadBuffer& buffer) noexcept
		{
			std::lock_guard<std::mutex> bufferLock(buffer.Mutex);
			SubmitPendingEvents(buffer.Events);
		}

		void FlushAllThreadBuffers() noexcept
		{
			std::vector<ThreadBuffer*> buffers;
			{
				std::lock_guard<std::mutex> lock(m_stateMutex);
				buffers = m_threadBuffers;
			}

			for (ThreadBuffer* buffer : buffers)
			{
				if (buffer != nullptr)
				{
					FlushThreadBuffer(*buffer);
				}
			}
		}

		void AppendEvent(const TraceEventRecord& event) noexcept
		{
			ThreadBuffer& buffer = GetThreadBuffer();
			std::lock_guard<std::mutex> bufferLock(buffer.Mutex);
			buffer.Events.push_back(event);
			if (buffer.Events.size() >= kThreadBufferCapacity)
			{
				SubmitPendingEvents(buffer.Events);
			}
		}

		void SubmitPendingEvents(std::vector<TraceEventRecord>& bufferEvents) noexcept
		{
			if (bufferEvents.empty())
			{
				return;
			}

			std::lock_guard<std::mutex> lock(m_stateMutex);
			SubmitPendingEventsUnlocked(bufferEvents);
		}

		void SubmitPendingEventsUnlocked(std::vector<TraceEventRecord>& bufferEvents) noexcept
		{
			if (bufferEvents.empty())
			{
				return;
			}

			m_events.insert(m_events.end(), std::make_move_iterator(bufferEvents.begin()), std::make_move_iterator(bufferEvents.end()));
			bufferEvents.clear();
		}

		std::uint32_t InternName(std::string_view name) noexcept
		{
			std::lock_guard<std::mutex> lock(m_stateMutex);
			const auto it = m_nameIds.find(name);
			if (it != m_nameIds.end())
			{
				return it->second;
			}

			const std::uint32_t nextId = m_nextNameId++;
			m_names.emplace_back(name);
			m_nameIds.emplace(m_names.back(), nextId);
			return nextId;
		}

		std::shared_ptr<spdlog::logger> ResolveMirrorLogger() const noexcept
		{
			if (m_logMirrorLogger != nullptr)
			{
				return m_logMirrorLogger;
			}
			return Logging::GetOrCreateLogger("Diagnostics.Trace");
		}

		std::string LookupName(std::uint32_t nameId)
		{
			std::lock_guard<std::mutex> lock(m_stateMutex);
			if (nameId < m_names.size())
			{
				return m_names[nameId];
			}
			return {};
		}

		std::uint64_t GetTimestampMicroseconds() const noexcept
		{
			return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - m_sessionStart).count());
		}

		void FlushInternal(bool keepSessionActive, bool exportTrace) noexcept
		{
			if (!IsSessionActive())
			{
				return;
			}

			FlushAllThreadBuffers();

			std::vector<TraceEventRecord> eventsCopy;
			std::vector<std::string> namesCopy;
			std::filesystem::path outputPath;
			{
				std::lock_guard<std::mutex> lock(m_stateMutex);
				eventsCopy = m_events;
				namesCopy = m_names;
				outputPath = m_outputPath;
				if (!keepSessionActive)
				{
					m_sessionActive.store(false, std::memory_order_release);
				}
			}

			if (exportTrace)
			{
				ExportTrace(eventsCopy, namesCopy, outputPath);
			}

			if (!keepSessionActive)
			{
				std::lock_guard<std::mutex> lock(m_stateMutex);
				m_events.clear();
				m_nameIds.clear();
				m_names.clear();
				m_names.push_back({});
				m_nextNameId = 1;
				m_logMirrorLogger.reset();
				m_logMirroringEnabled = false;
				m_outputPath.clear();
				m_exportOnEnd = true;
			}
		}

		void ExportTrace(
			std::vector<TraceEventRecord>& events,
			const std::vector<std::string>& names,
			const std::filesystem::path& outputPath) const noexcept
		{
			std::stable_sort(
			    events.begin(),
			    events.end(),
			    [](const TraceEventRecord& left, const TraceEventRecord& right)
			    {
				    if (left.TimestampMicroseconds != right.TimestampMicroseconds)
				    {
					    return left.TimestampMicroseconds < right.TimestampMicroseconds;
				    }

				    if (left.ThreadId != right.ThreadId)
				    {
					    return left.ThreadId < right.ThreadId;
				    }

				    return left.Phase < right.Phase;
			    });

			std::string output;
			output.reserve(events.size() * 128);
			output += "{\"traceEvents\":[";

			for (std::size_t eventIndex = 0; eventIndex < events.size(); ++eventIndex)
			{
				const TraceEventRecord& event = events[eventIndex];
				if (eventIndex > 0)
				{
					output.push_back(',');
				}

				output += "{\"name\":\"";
				AppendEscapedJson(output, ResolveName(names, event.NameId));
				output += "\",\"cat\":\"";
				AppendEscapedJson(output, ToCategoryString(event.Category));
				output += "\",\"ph\":\"";
				output.push_back(event.Phase);
				output += "\",\"pid\":1,\"tid\":";
				output += std::to_string(event.ThreadId);
				output += ",\"ts\":";
				output += std::to_string(event.TimestampMicroseconds);
				if (event.Phase == 'i')
				{
					output += ",\"s\":\"t\"";
				}
				output.push_back('}');
			}

			output += "],\"displayTimeUnit\":\"ms\"}";

			std::string errorMessage;
			const std::filesystem::path normalizedOutputPath = outputPath.empty() ? std::filesystem::path{"logs/trace.json"} : outputPath;
			if (!Files::TryWriteAllText(normalizedOutputPath, output, errorMessage))
			{
				const std::shared_ptr<spdlog::logger> logger =
				    m_logMirrorLogger != nullptr ? m_logMirrorLogger : Logging::GetOrCreateLogger("Diagnostics.Trace");
				if (logger != nullptr)
				{
					SPDLOG_LOGGER_ERROR(logger, "Failed to export trace to '{}': {}", normalizedOutputPath.string(), errorMessage);
				}
			}
		}

		static std::string_view ResolveName(const std::vector<std::string>& names, std::uint32_t nameId) noexcept
		{
			return nameId < names.size() ? std::string_view(names[nameId]) : std::string_view{};
		}

		static std::string_view ToCategoryString(ETraceCategory category) noexcept
		{
			switch (category)
			{
				case ETraceCategory::Application:
					return "Application";
				case ETraceCategory::Editor:
					return "Editor";
				case ETraceCategory::GameFramework:
					return "GameFramework";
				case ETraceCategory::Renderer:
					return "Renderer";
				case ETraceCategory::Tools:
					return "Tools";
				case ETraceCategory::Default:
				default:
					return "Default";
			}
		}

		static void AppendEscapedJson(std::string& destination, std::string_view text)
		{
			for (const char character : text)
			{
				switch (character)
				{
					case '\\':
						destination += "\\\\";
						break;
					case '"':
						destination += "\\\"";
						break;
					case '\n':
						destination += "\\n";
						break;
					case '\r':
						destination += "\\r";
						break;
					case '\t':
						destination += "\\t";
						break;
					default:
						destination.push_back(character);
						break;
				}
			}
		}

		std::atomic<bool> m_sessionActive{false};
		std::mutex m_stateMutex;
		NameIdMap m_nameIds;
		std::vector<std::string> m_names;
		std::vector<TraceEventRecord> m_events;
		std::vector<ThreadBuffer*> m_threadBuffers;
		std::shared_ptr<spdlog::logger> m_logMirrorLogger;
		std::filesystem::path m_outputPath;
		TimePoint m_sessionStart{};
		std::uint32_t m_nextNameId = 1;
		bool m_exportOnEnd = true;
		bool m_logMirroringEnabled = false;
	};

	ScopedTrace::ScopedTrace(std::string_view name, ETraceCategory category) noexcept : m_category(category)
	{
		if (name.empty())
		{
			return;
		}

		m_beginTime = std::chrono::steady_clock::now();
		m_liveProfilerActive = LiveProfiler::Get().IsEnabled();
		if (m_liveProfilerActive)
		{
			LiveProfiler::Get().BeginCpuScope(name);
		}

		m_traceNameId = TraceService::Get().BeginScope(name, category, m_traceBeginTimestampMicroseconds);
	}

	ScopedTrace::ScopedTrace(const DiagnosticName& name, ETraceCategory category) noexcept : ScopedTrace(name.GetCanonicalName(), category)
	{
	}

	ScopedTrace::~ScopedTrace() noexcept
	{
		if (m_traceNameId != 0)
		{
			TraceService::Get().EndScope(m_traceNameId, m_category, m_traceBeginTimestampMicroseconds);
		}

		if (m_liveProfilerActive)
		{
			const auto elapsed = std::chrono::steady_clock::now() - m_beginTime;
			const auto elapsedMicroseconds = static_cast<std::uint64_t>(
			    std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
			LiveProfiler::Get().EndCpuScope(elapsedMicroseconds);
		}
	}

	void BeginTraceSession(const TraceSessionConfig& config) noexcept
	{
		TraceService::Get().BeginSession(config);
	}

	void EndTraceSession() noexcept
	{
		TraceService::Get().EndSession();
	}

	void FlushTrace() noexcept
	{
		TraceService::Get().Flush();
	}

	bool IsTraceSessionActive() noexcept
	{
		return TraceService::Get().IsSessionActive();
	}
}