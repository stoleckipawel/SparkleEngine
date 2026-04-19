#pragma once

#include "Core/Public/CoreAPI.h"
#include "Core/Public/CoreMacros.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace Engine::Diagnostics
{
	enum class ETraceCategory : std::uint8_t
	{
		Default = 0,
		Application = 1,
		Editor = 2,
		GameFramework = 3,
		Renderer = 4,
		Tools = 5,
	};

	class SPARKLE_CORE_API DiagnosticName final
	{
	  public:
		constexpr DiagnosticName() noexcept = default;
		constexpr explicit DiagnosticName(std::string_view canonicalName) noexcept : m_canonicalName(canonicalName) {}

		constexpr std::string_view GetCanonicalName() const noexcept { return m_canonicalName; }
		constexpr bool IsEmpty() const noexcept { return m_canonicalName.empty(); }
		constexpr explicit operator bool() const noexcept { return !IsEmpty(); }

	  private:
		std::string_view m_canonicalName{};
	};

	struct SPARKLE_CORE_API TraceSessionConfig
	{
		std::filesystem::path OutputPath{"logs/trace.json"};
		bool ExportOnEnd = true;
		bool EnableLogMirroring = false;
	};

	// RAII scope. Routes timing data to two independent sinks:
	//   1. TraceService -- Chrome trace JSON export, opt-in via BeginTraceSession.
	//   2. LiveProfiler -- in-memory hierarchical store, always-on for the editor
	//                      profiler panel.
	// Holds no heap allocations.
	class SPARKLE_CORE_API ScopedTrace final
	{
	  public:
		ScopedTrace(std::string_view name, ETraceCategory category = ETraceCategory::Default) noexcept;
		ScopedTrace(const DiagnosticName& name, ETraceCategory category = ETraceCategory::Default) noexcept;
		~ScopedTrace() noexcept;

		ScopedTrace(const ScopedTrace&) = delete;
		ScopedTrace& operator=(const ScopedTrace&) = delete;
		ScopedTrace(ScopedTrace&&) = delete;
		ScopedTrace& operator=(ScopedTrace&&) = delete;

		bool IsActive() const noexcept { return m_traceNameId != 0 || m_liveProfilerActive; }

	  private:
		std::chrono::steady_clock::time_point m_beginTime{};
		std::uint64_t m_traceBeginTimestampMicroseconds = 0;
		std::uint32_t m_traceNameId = 0;
		ETraceCategory m_category = ETraceCategory::Default;
		bool m_liveProfilerActive = false;
	};

	SPARKLE_CORE_API void BeginTraceSession(const TraceSessionConfig& config = TraceSessionConfig{}) noexcept;
	SPARKLE_CORE_API void EndTraceSession() noexcept;
	SPARKLE_CORE_API void FlushTrace() noexcept;
	SPARKLE_CORE_API bool IsTraceSessionActive() noexcept;
}

#ifndef SPARKLE_CPU_SCOPE
#define SPARKLE_CPU_SCOPE(name) ::Engine::Diagnostics::ScopedTrace SPARKLE_PP_CONCAT(_sparkleCpuScope_, __LINE__){(name)}
#endif