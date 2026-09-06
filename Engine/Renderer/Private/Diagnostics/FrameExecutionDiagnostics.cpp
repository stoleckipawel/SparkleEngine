#include "PCH.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"

#include "Commands/RenderCommandContext.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <algorithm>
#include <utility>

static const auto g_frameExecutionDiagnosticsLogger = Logging::GetOrCreateLogger("Renderer.FrameExecutionDiagnostics");

FrameExecutionDiagnostics::FrameExecutionDiagnostics(RenderDiagnostics& backendDiagnostics) noexcept :
    m_backendDiagnostics(&backendDiagnostics),
    m_timingDiagnostics(backendDiagnostics.GetTimingDiagnostics())
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

const std::vector<ResolvedGpuTiming>& FrameExecutionDiagnostics::GetResolvedTimings() const noexcept
{
	return m_resolvedTimers;
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

	return ScopedGpuEvent(commands, label, color);
}

ScopedGpuTimer FrameExecutionDiagnostics::BeginTimer(RenderCommandContext& commands, std::string_view label) noexcept
{
	if (!CVarRendererDiagnosticGpuTiming.Get() || !SupportsTimestampQueries() || label.empty())
	{
		return {};
	}

	const ERhiQueueType queueType = commands.GetRenderCommandList().GetQueueType();
	const RhiTimestampQueryHandle beginQuery = AllocateTimestampQuery(queueType);
	const RhiTimestampQueryHandle endQuery = AllocateTimestampQuery(queueType);
	if (!beginQuery || !endQuery)
	{
		ReleaseTimestampQuery(beginQuery);
		ReleaseTimestampQuery(endQuery);
		return {};
	}

	return ScopedGpuTimer(*this, commands, std::string(label), beginQuery, endQuery, queueType);
}

ScopedGpuScope FrameExecutionDiagnostics::BeginGpuScope(
    RenderCommandContext& commands,
    std::string_view label,
    RhiDiagnosticLabelColor color) noexcept
{
	return ScopedGpuScope{BeginGpuEvent(commands, label, color), BeginTimer(commands, label)};
}

void FrameExecutionDiagnostics::InsertGpuMarker(
    RenderCommandContext& commands,
    std::string_view label,
    RhiDiagnosticLabelColor color) const noexcept
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
	std::vector<GpuTimingScope> recordedTimers;
	{
		std::lock_guard lock(m_recordedTimersMutex);
		recordedTimers.swap(m_recordedTimers);
	}

	if (recordedTimers.empty())
	{
		return;
	}
	if (m_timingDiagnostics == nullptr)
	{
		Diagnostics::Fatal(
		    g_frameExecutionDiagnosticsLogger,
		    __FILE__,
		    __LINE__,
		    "Recorded GPU timings have no backend timing diagnostics service.");
	}

	for (const GpuTimingScope& record : recordedTimers)
	{
		const double timestampPeriodNanoseconds = m_timingDiagnostics->GetTimestampPeriodNanoseconds(record.BeginQuery);
		const std::uint32_t timestampValidBits = m_timingDiagnostics->GetTimestampValidBits(record.BeginQuery);
		std::uint64_t beginTicks = 0;
		std::uint64_t endTicks = 0;
		if (!m_timingDiagnostics->TryResolveTimestamp(record.BeginQuery, beginTicks)
		    || !m_timingDiagnostics->TryResolveTimestamp(record.EndQuery, endTicks))
		{
			Diagnostics::Fatal(
			    g_frameExecutionDiagnosticsLogger,
			    __FILE__,
			    __LINE__,
			    "A GPU timestamp was unavailable after its frame slot retired.");
		}

		std::uint64_t durationTicks = endTicks - beginTicks;
		if (endTicks < beginTicks)
		{
			if (timestampValidBits == 0 || timestampValidBits >= 64)
			{
				Diagnostics::Fatal(
				    g_frameExecutionDiagnosticsLogger,
				    __FILE__,
				    __LINE__,
				    "A GPU timestamp wrapped without a finite native timestamp bit range.");
			}
			durationTicks = (std::uint64_t{1} << timestampValidBits) - beginTicks + endTicks;
		}
		const double durationMilliseconds = static_cast<double>(durationTicks) * timestampPeriodNanoseconds / 1'000'000.0;
		m_resolvedTimers.push_back(
		    ResolvedGpuTiming{
		        .Label = record.Label,
		        .BeginTicks = beginTicks,
		        .EndTicks = endTicks,
		        .DurationTicks = durationTicks,
		        .DurationMilliseconds = durationMilliseconds,
		        .QueueType = record.QueueType,
		        .Depth = record.Depth});
	}

	// Records are pushed in scope-end order, so sort them back into begin order before
	// exposing the resolved timing stream.
	std::sort(
	    m_resolvedTimers.begin(),
	    m_resolvedTimers.end(),
	    [](const ResolvedGpuTiming& lhs, const ResolvedGpuTiming& rhs) noexcept
	    {
		    if (lhs.QueueType != rhs.QueueType)
		    {
			    return lhs.QueueType < rhs.QueueType;
		    }
		    if (lhs.BeginTicks != rhs.BeginTicks)
		    {
			    return lhs.BeginTicks < rhs.BeginTicks;
		    }
		    return lhs.Depth < rhs.Depth;
	    });

	for (const GpuTimingScope& record : recordedTimers)
	{
		ReleaseTimestampQuery(record.BeginQuery);
		ReleaseTimestampQuery(record.EndQuery);
	}
}

RhiTimestampQueryHandle FrameExecutionDiagnostics::AllocateTimestampQuery(ERhiQueueType queueType) noexcept
{
	return m_timingDiagnostics != nullptr ? m_timingDiagnostics->AllocateTimestampQuery(queueType) : RhiTimestampQueryHandle{};
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
    ERhiQueueType queueType,
    std::uint16_t depth) noexcept
{
	std::lock_guard lock(m_recordedTimersMutex);
	m_recordedTimers.push_back(
	    GpuTimingScope{.Label = std::move(label), .BeginQuery = beginQuery, .EndQuery = endQuery, .QueueType = queueType, .Depth = depth});
}

void FrameExecutionDiagnostics::ResetRecordedTimers() noexcept
{
	std::vector<GpuTimingScope> recordedTimers;
	{
		std::lock_guard lock(m_recordedTimersMutex);
		recordedTimers.swap(m_recordedTimers);
	}

	for (const GpuTimingScope& record : recordedTimers)
	{
		ReleaseTimestampQuery(record.BeginQuery);
		ReleaseTimestampQuery(record.EndQuery);
	}
}
