#pragma once

#include "RHI/Public/Diagnostics/RhiDiagnostics.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

class RenderCommandContext;
class FrameGraphExecutionDiagnostics;

struct GpuTimingScope
{
	std::string Label;
	RhiTimestampQueryHandle BeginQuery = {};
	RhiTimestampQueryHandle EndQuery = {};
	ERhiQueueType QueueType = ERhiQueueType::Graphics;
	std::uint16_t Depth = 0;
};

struct ResolvedGpuTiming
{
	std::string Label;
	std::uint64_t BeginTicks = 0;
	std::uint64_t EndTicks = 0;
	std::uint64_t DurationTicks = 0;
	double DurationMilliseconds = 0.0;
	ERhiQueueType QueueType = ERhiQueueType::Graphics;
	std::uint16_t Depth = 0;
};

class FrameExecutionDiagnostics;

class ScopedGpuEvent final
{
public:
	ScopedGpuEvent() noexcept = default;
	~ScopedGpuEvent() noexcept;

	ScopedGpuEvent(const ScopedGpuEvent&) = delete;
	ScopedGpuEvent& operator=(const ScopedGpuEvent&) = delete;
	ScopedGpuEvent(ScopedGpuEvent&& other) noexcept;
	ScopedGpuEvent& operator=(ScopedGpuEvent&& other) noexcept;

	bool IsActive() const noexcept;

private:
	friend class FrameExecutionDiagnostics;

	ScopedGpuEvent(RenderCommandContext& commands, std::string label, RhiDiagnosticLabelColor color) noexcept;
	void Reset() noexcept;

	RenderCommandContext* m_commands = nullptr;
};

class ScopedGpuTimer final
{
public:
	ScopedGpuTimer() noexcept = default;
	~ScopedGpuTimer() noexcept;

	ScopedGpuTimer(const ScopedGpuTimer&) = delete;
	ScopedGpuTimer& operator=(const ScopedGpuTimer&) = delete;
	ScopedGpuTimer(ScopedGpuTimer&& other) noexcept;
	ScopedGpuTimer& operator=(ScopedGpuTimer&& other) noexcept;

	bool IsActive() const noexcept;

private:
	friend class FrameExecutionDiagnostics;

	ScopedGpuTimer(
	    FrameExecutionDiagnostics& owner,
	    RenderCommandContext& commands,
	    std::string label,
	    RhiTimestampQueryHandle beginQuery,
	    RhiTimestampQueryHandle endQuery,
	    ERhiQueueType queueType) noexcept;
	void Reset() noexcept;

	FrameExecutionDiagnostics* m_owner = nullptr;
	RenderCommandContext* m_commands = nullptr;
	std::string m_label;
	RhiTimestampQueryHandle m_beginQuery = {};
	RhiTimestampQueryHandle m_endQuery = {};
	ERhiQueueType m_queueType = ERhiQueueType::Graphics;
	std::uint16_t m_depth = 0;
};

class ScopedGpuScope final
{
public:
	ScopedGpuScope() noexcept = default;
	ScopedGpuScope(ScopedGpuEvent eventScope, ScopedGpuTimer timerScope) noexcept;
	~ScopedGpuScope() noexcept = default;

	ScopedGpuScope(const ScopedGpuScope&) = delete;
	ScopedGpuScope& operator=(const ScopedGpuScope&) = delete;
	ScopedGpuScope(ScopedGpuScope&& other) noexcept;
	ScopedGpuScope& operator=(ScopedGpuScope&& other) noexcept;

	bool IsActive() const noexcept;

private:
	ScopedGpuEvent m_eventScope;
	ScopedGpuTimer m_timerScope;
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

	bool IsGpuTimingAvailable() const noexcept;
	ScopedGpuScope BeginGpuScope(RenderCommandContext& commands, std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept;
	void ResolveTimings() noexcept;

	const std::vector<ResolvedGpuTiming>& GetResolvedTimings() const noexcept;

private:
	friend class ScopedGpuTimer;
	friend class FrameGraphExecutionDiagnostics;

	bool SupportsGpuEvents() const noexcept;
	bool SupportsTimestampQueries() const noexcept;
	ScopedGpuEvent BeginGpuEvent(RenderCommandContext& commands, std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept;
	ScopedGpuTimer BeginTimer(RenderCommandContext& commands, std::string_view label) noexcept;
	void InsertGpuMarker(RenderCommandContext& commands, std::string_view label, RhiDiagnosticLabelColor color = {}) const noexcept;
	RhiTimestampQueryHandle AllocateTimestampQuery(ERhiQueueType queueType) noexcept;
	void ReleaseTimestampQuery(RhiTimestampQueryHandle query) noexcept;
	bool WriteTimestamp(RenderCommandContext& commands, RhiTimestampQueryHandle query) noexcept;
	void RecordCompletedTimer(
	    std::string label,
	    RhiTimestampQueryHandle beginQuery,
	    RhiTimestampQueryHandle endQuery,
	    ERhiQueueType queueType,
	    std::uint16_t depth) noexcept;
	void ResetRecordedTimers() noexcept;

	RenderDiagnostics* m_backendDiagnostics = nullptr;
	RenderTimingDiagnostics* m_timingDiagnostics = nullptr;
	std::vector<GpuTimingScope> m_recordedTimers;
	std::vector<ResolvedGpuTiming> m_resolvedTimers;
	std::mutex m_recordedTimersMutex;
};
