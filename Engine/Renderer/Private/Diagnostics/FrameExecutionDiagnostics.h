#pragma once

#include "FrameGraph/FrameGraphPassFlags.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class RenderCommandContext;

struct GpuTimingScope
{
	std::string Label;
	RhiTimestampQueryHandle BeginQuery = {};
	RhiTimestampQueryHandle EndQuery = {};
	std::uint16_t Depth = 0;
};

struct ResolvedGpuTiming
{
	std::string Label;
	std::uint64_t BeginTicks = 0;
	std::uint64_t EndTicks = 0;
	std::uint64_t DurationTicks = 0;
	double DurationMilliseconds = 0.0;
	std::uint16_t Depth = 0;
};

class FrameExecutionDiagnostics;

class ScopedGpuEvent final
{
  public:
	ScopedGpuEvent() noexcept = default;
	ScopedGpuEvent(RenderCommandContext& commands, std::string label, RhiDiagnosticLabelColor color) noexcept;
	~ScopedGpuEvent() noexcept;

	ScopedGpuEvent(const ScopedGpuEvent&) = delete;
	ScopedGpuEvent& operator=(const ScopedGpuEvent&) = delete;
	ScopedGpuEvent(ScopedGpuEvent&& other) noexcept;
	ScopedGpuEvent& operator=(ScopedGpuEvent&& other) noexcept;

	bool IsActive() const noexcept { return m_commands != nullptr; }

  private:
	void Reset() noexcept;

	RenderCommandContext* m_commands = nullptr;
};

class ScopedGpuTimer final
{
  public:
	ScopedGpuTimer() noexcept = default;
	ScopedGpuTimer(
	    FrameExecutionDiagnostics& owner,
	    RenderCommandContext& commands,
	    std::string label,
	    RhiTimestampQueryHandle beginQuery,
	    RhiTimestampQueryHandle endQuery) noexcept;
	~ScopedGpuTimer() noexcept;

	ScopedGpuTimer(const ScopedGpuTimer&) = delete;
	ScopedGpuTimer& operator=(const ScopedGpuTimer&) = delete;
	ScopedGpuTimer(ScopedGpuTimer&& other) noexcept;
	ScopedGpuTimer& operator=(ScopedGpuTimer&& other) noexcept;

	bool IsActive() const noexcept { return m_owner != nullptr; }

  private:
	void Reset() noexcept;

	FrameExecutionDiagnostics* m_owner = nullptr;
	RenderCommandContext* m_commands = nullptr;
	std::string m_label;
	RhiTimestampQueryHandle m_beginQuery = {};
	RhiTimestampQueryHandle m_endQuery = {};
	std::uint16_t m_depth = 0;
	bool m_depthAccounted = false;
};

class FrameExecutionDiagnostics final
{
  public:
	explicit FrameExecutionDiagnostics(RenderDiagnostics& backendDiagnostics) noexcept;
	~FrameExecutionDiagnostics() noexcept;

	FrameExecutionDiagnostics(const FrameExecutionDiagnostics&) = delete;
	FrameExecutionDiagnostics& operator=(const FrameExecutionDiagnostics&) = delete;
	FrameExecutionDiagnostics(FrameExecutionDiagnostics&&) = delete;
	FrameExecutionDiagnostics& operator=(FrameExecutionDiagnostics&&) = delete;

	RenderDiagnostics& GetBackendDiagnostics() noexcept { return *m_backendDiagnostics; }
	const RenderDiagnostics& GetBackendDiagnostics() const noexcept { return *m_backendDiagnostics; }

	bool SupportsGpuEvents() const noexcept;
	bool SupportsTimestampQueries() const noexcept;

	ScopedGpuEvent BeginGpuEvent(RenderCommandContext& commands, std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept;
	ScopedGpuEvent BeginGpuEvent(
	    RenderCommandContext& commands,
	    const Diagnostics::DiagnosticName& name,
	    RhiDiagnosticLabelColor color = {}) noexcept;
	ScopedGpuTimer BeginTimer(RenderCommandContext& commands, std::string_view label) noexcept;
	ScopedGpuTimer BeginTimer(RenderCommandContext& commands, const Diagnostics::DiagnosticName& name) noexcept;
	void InsertGpuMarker(RenderCommandContext& commands, std::string_view label, RhiDiagnosticLabelColor color = {}) const noexcept;
	void InsertGpuMarker(RenderCommandContext& commands, const Diagnostics::DiagnosticName& name, RhiDiagnosticLabelColor color = {})
	    const noexcept;
	void ResolveTimings() noexcept;

	const std::vector<GpuTimingScope>& GetRecordedTimings() const noexcept { return m_recordedTimers; }
	const std::vector<ResolvedGpuTiming>& GetResolvedTimings() const noexcept { return m_resolvedTimers; }

  private:
	friend class ScopedGpuTimer;

	RhiTimestampQueryHandle AllocateTimestampQuery() noexcept;
	void ReleaseTimestampQuery(RhiTimestampQueryHandle query) noexcept;
	bool WriteTimestamp(RenderCommandContext& commands, RhiTimestampQueryHandle query) noexcept;
	void RecordCompletedTimer(
	    std::string label,
	    RhiTimestampQueryHandle beginQuery,
	    RhiTimestampQueryHandle endQuery,
	    std::uint16_t depth) noexcept;
	std::uint16_t AcquireTimerDepth() noexcept;
	void ReleaseTimerDepth() noexcept;
	void ResetRecordedTimers() noexcept;

	RenderDiagnostics* m_backendDiagnostics = nullptr;
	RenderTimingDiagnostics* m_timingDiagnostics = nullptr;
	std::vector<GpuTimingScope> m_recordedTimers;
	std::vector<ResolvedGpuTiming> m_resolvedTimers;
	std::uint16_t m_openTimerCount = 0;
};

#ifndef SPARKLE_DIAGNOSTIC_NAME
	#define SPARKLE_DIAGNOSTIC_NAME(name) \
		::Diagnostics::DiagnosticName     \
		{                                 \
			(name)                        \
		}
#endif