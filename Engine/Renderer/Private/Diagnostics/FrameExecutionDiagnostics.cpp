#include "PCH.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"

#include "Commands/RenderCommandContext.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <algorithm>
#include <utility>

FrameExecutionDiagnostics::FrameExecutionDiagnostics(RenderDiagnostics& backendDiagnostics) noexcept :
    m_backendDiagnostics(&backendDiagnostics), m_timingDiagnostics(backendDiagnostics.GetTimingDiagnostics())
{
}

FrameExecutionDiagnostics::~FrameExecutionDiagnostics() noexcept
{
	ResetRecordedTimers();
}

bool FrameExecutionDiagnostics::SupportsGpuEvents() const noexcept
{
	return m_backendDiagnostics != nullptr && m_backendDiagnostics->GetCapabilities().SupportsGpuEvents;
}

bool FrameExecutionDiagnostics::SupportsTimestampQueries() const noexcept
{
	return m_timingDiagnostics != nullptr && m_timingDiagnostics->SupportsTimestampQueries();
}

bool FrameExecutionDiagnostics::IsGpuTimingAvailable() const noexcept
{
	return CVarRendererDiagnosticGpuTiming.Get() && SupportsTimestampQueries();
}

ScopedGpuEvent FrameExecutionDiagnostics::BeginGpuEvent(
    RenderCommandContext& commands,
    std::string_view label,
    RhiDiagnosticLabelColor color) noexcept
{
	if (!SupportsGpuEvents() || label.empty())
	{
		return {};
	}

	return ScopedGpuEvent(commands, std::string(label), color);
}

ScopedGpuTimer FrameExecutionDiagnostics::BeginTimer(RenderCommandContext& commands, std::string_view label) noexcept
{
	if (!CVarRendererDiagnosticGpuTiming.Get() || !SupportsTimestampQueries() || label.empty())
	{
		return {};
	}

	const RhiTimestampQueryHandle beginQuery = AllocateTimestampQuery();
	const RhiTimestampQueryHandle endQuery = AllocateTimestampQuery();
	if (!beginQuery || !endQuery)
	{
		ReleaseTimestampQuery(beginQuery);
		ReleaseTimestampQuery(endQuery);
		return {};
	}

	return ScopedGpuTimer(*this, commands, std::string(label), beginQuery, endQuery);
}

ScopedGpuScope FrameExecutionDiagnostics::BeginGpuScope(
    RenderCommandContext& commands,
    std::string_view label,
    RhiDiagnosticLabelColor color) noexcept
{
	return ScopedGpuScope{BeginGpuEvent(commands, label, color), BeginTimer(commands, label)};
}

ScopedGpuScope FrameExecutionDiagnostics::BeginGpuScope(
    RenderCommandContext& commands,
    const Diagnostics::DiagnosticName& name,
    RhiDiagnosticLabelColor color) noexcept
{
	return BeginGpuScope(commands, name.GetCanonicalName(), color);
}

void FrameExecutionDiagnostics::InsertGpuMarker(RenderCommandContext& commands, std::string_view label, RhiDiagnosticLabelColor color)
    const noexcept
{
	if (!SupportsGpuEvents() || label.empty())
	{
		return;
	}

	commands.InsertDiagnosticMarker(label, color);
}

void FrameExecutionDiagnostics::ResolveTimings() noexcept
{
	m_resolvedTimers.clear();
	if (m_timingDiagnostics == nullptr || m_recordedTimers.empty())
	{
		ResetRecordedTimers();
		return;
	}

	const std::uint64_t frequencyHz = m_timingDiagnostics->GetTimestampFrequencyHz();
	for (const GpuTimingScope& record : m_recordedTimers)
	{
		std::uint64_t beginTicks = 0;
		std::uint64_t endTicks = 0;
		if (!m_timingDiagnostics->TryResolveTimestamp(record.BeginQuery, beginTicks) ||
		    !m_timingDiagnostics->TryResolveTimestamp(record.EndQuery, endTicks) || endTicks < beginTicks)
		{
			continue;
		}

		const std::uint64_t durationTicks = endTicks - beginTicks;
		const double durationMilliseconds =
		    frequencyHz > 0 ? (static_cast<double>(durationTicks) * 1000.0) / static_cast<double>(frequencyHz) : 0.0;
		m_resolvedTimers.push_back(
		    ResolvedGpuTiming{
		        .Label = record.Label,
		        .BeginTicks = beginTicks,
		        .EndTicks = endTicks,
		        .DurationTicks = durationTicks,
		        .DurationMilliseconds = durationMilliseconds,
		        .Depth = record.Depth});
	}

	// Records are pushed in scope-end order (inner timers end before their parents),
	// but downstream consumers (LiveProfiler hierarchy reconstruction) require
	// begin-order entries to interpret the depth stream correctly. Sort ascending by
	// begin tick; tie-break by depth so a parent precedes a child that begins on the
	// same tick.
	std::sort(
	    m_resolvedTimers.begin(),
	    m_resolvedTimers.end(),
	    [](const ResolvedGpuTiming& lhs, const ResolvedGpuTiming& rhs) noexcept
	    {
		    if (lhs.BeginTicks != rhs.BeginTicks)
		    {
			    return lhs.BeginTicks < rhs.BeginTicks;
		    }
		    return lhs.Depth < rhs.Depth;
	    });

	ResetRecordedTimers();
}

RhiTimestampQueryHandle FrameExecutionDiagnostics::AllocateTimestampQuery() noexcept
{
	return m_timingDiagnostics != nullptr ? m_timingDiagnostics->AllocateTimestampQuery() : RhiTimestampQueryHandle{};
}

void FrameExecutionDiagnostics::ReleaseTimestampQuery(RhiTimestampQueryHandle query) noexcept
{
	if (m_timingDiagnostics != nullptr && query)
	{
		m_timingDiagnostics->ReleaseTimestampQuery(query);
	}
}

bool FrameExecutionDiagnostics::WriteTimestamp(RenderCommandContext& commands, RhiTimestampQueryHandle query) noexcept
{
	return m_timingDiagnostics != nullptr && query && m_timingDiagnostics->WriteTimestamp(commands.GetRenderCommandList(), query);
}

void FrameExecutionDiagnostics::RecordCompletedTimer(
    std::string label,
    RhiTimestampQueryHandle beginQuery,
    RhiTimestampQueryHandle endQuery,
    std::uint16_t depth) noexcept
{
	m_recordedTimers.push_back(GpuTimingScope{.Label = std::move(label), .BeginQuery = beginQuery, .EndQuery = endQuery, .Depth = depth});
}

std::uint16_t FrameExecutionDiagnostics::AcquireTimerDepth() noexcept
{
	return m_openTimerCount++;
}

void FrameExecutionDiagnostics::ReleaseTimerDepth() noexcept
{
	if (m_openTimerCount > 0)
	{
		--m_openTimerCount;
	}
}

void FrameExecutionDiagnostics::ResetRecordedTimers() noexcept
{
	for (const GpuTimingScope& record : m_recordedTimers)
	{
		ReleaseTimestampQuery(record.BeginQuery);
		ReleaseTimestampQuery(record.EndQuery);
	}

	m_recordedTimers.clear();
}
