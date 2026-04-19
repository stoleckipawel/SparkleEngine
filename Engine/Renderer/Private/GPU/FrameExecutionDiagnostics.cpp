#include "PCH.h"

#include "GPU/FrameExecutionDiagnostics.h"

#include "GPU/CommandContext.h"

#include <utility>

ScopedGpuEvent::ScopedGpuEvent(CommandContext& commands, std::string label, RhiDiagnosticLabelColor color) noexcept :
	m_commands(&commands)
{
	if (label.empty())
	{
		m_commands = nullptr;
		return;
	}

	commands.BeginDiagnosticScope(label, color);
}

ScopedGpuEvent::~ScopedGpuEvent() noexcept
{
	Reset();
}

ScopedGpuEvent::ScopedGpuEvent(ScopedGpuEvent&& other) noexcept : m_commands(other.m_commands)
{
	other.m_commands = nullptr;
}

ScopedGpuEvent& ScopedGpuEvent::operator=(ScopedGpuEvent&& other) noexcept
{
	if (this != &other)
	{
		Reset();
		m_commands = other.m_commands;
		other.m_commands = nullptr;
	}

	return *this;
}

void ScopedGpuEvent::Reset() noexcept
{
	if (m_commands != nullptr)
	{
		m_commands->EndDiagnosticScope();
		m_commands = nullptr;
	}
}

ScopedGpuTimer::ScopedGpuTimer(
	FrameExecutionDiagnostics& owner,
	CommandContext& commands,
	std::string label,
	RhiTimestampQueryHandle beginQuery,
	RhiTimestampQueryHandle endQuery) noexcept :
	m_owner(&owner), m_commands(&commands), m_label(std::move(label)), m_beginQuery(beginQuery), m_endQuery(endQuery)
{
	if (m_label.empty() || !m_beginQuery || !m_endQuery || !m_owner->WriteTimestamp(commands, m_beginQuery))
	{
		Reset();
		return;
	}

	m_depth = m_owner->AcquireTimerDepth();
	m_depthAccounted = true;
}

ScopedGpuTimer::~ScopedGpuTimer() noexcept
{
	if (m_owner == nullptr || m_commands == nullptr)
	{
		return;
	}

	if (!m_owner->WriteTimestamp(*m_commands, m_endQuery))
	{
		Reset();
		return;
	}

	m_owner->RecordCompletedTimer(std::move(m_label), m_beginQuery, m_endQuery, m_depth);
	if (m_depthAccounted)
	{
		m_owner->ReleaseTimerDepth();
		m_depthAccounted = false;
	}
	m_owner = nullptr;
	m_commands = nullptr;
	m_beginQuery = {};
	m_endQuery = {};
}

ScopedGpuTimer::ScopedGpuTimer(ScopedGpuTimer&& other) noexcept :
	m_owner(other.m_owner),
	m_commands(other.m_commands),
	m_label(std::move(other.m_label)),
	m_beginQuery(other.m_beginQuery),
	m_endQuery(other.m_endQuery),
	m_depth(other.m_depth),
	m_depthAccounted(other.m_depthAccounted)
{
	other.m_owner = nullptr;
	other.m_commands = nullptr;
	other.m_beginQuery = {};
	other.m_endQuery = {};
	other.m_depth = 0;
	other.m_depthAccounted = false;
}

ScopedGpuTimer& ScopedGpuTimer::operator=(ScopedGpuTimer&& other) noexcept
{
	if (this != &other)
	{
		Reset();
		m_owner = other.m_owner;
		m_commands = other.m_commands;
		m_label = std::move(other.m_label);
		m_beginQuery = other.m_beginQuery;
		m_endQuery = other.m_endQuery;
		m_depth = other.m_depth;
		m_depthAccounted = other.m_depthAccounted;
		other.m_owner = nullptr;
		other.m_commands = nullptr;
		other.m_beginQuery = {};
		other.m_endQuery = {};
		other.m_depth = 0;
		other.m_depthAccounted = false;
	}

	return *this;
}

void ScopedGpuTimer::Reset() noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->ReleaseTimestampQuery(m_beginQuery);
		m_owner->ReleaseTimestampQuery(m_endQuery);
		if (m_depthAccounted)
		{
			m_owner->ReleaseTimerDepth();
		}
	}

	m_owner = nullptr;
	m_commands = nullptr;
	m_label.clear();
	m_beginQuery = {};
	m_endQuery = {};
	m_depth = 0;
	m_depthAccounted = false;
}

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

ScopedGpuEvent FrameExecutionDiagnostics::BeginGpuEvent(
	CommandContext& commands,
	std::string_view label,
	RhiDiagnosticLabelColor color) noexcept
{
	if (!SupportsGpuEvents() || label.empty())
	{
		return {};
	}

	return ScopedGpuEvent(commands, std::string(label), color);
}

ScopedGpuTimer FrameExecutionDiagnostics::BeginTimer(CommandContext& commands, std::string_view label) noexcept
{
	if (!SupportsTimestampQueries() || label.empty())
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

ScopedGpuEvent FrameExecutionDiagnostics::BeginGpuEvent(
	CommandContext& commands,
	const Engine::Diagnostics::DiagnosticName& name,
	RhiDiagnosticLabelColor color) noexcept
{
	return BeginGpuEvent(commands, name.GetCanonicalName(), color);
}

ScopedGpuTimer FrameExecutionDiagnostics::BeginTimer(
	CommandContext& commands,
	const Engine::Diagnostics::DiagnosticName& name) noexcept
{
	return BeginTimer(commands, name.GetCanonicalName());
}

void FrameExecutionDiagnostics::InsertGpuMarker(
	CommandContext& commands,
	std::string_view label,
	RhiDiagnosticLabelColor color) const noexcept
{
	if (!SupportsGpuEvents() || label.empty())
	{
		return;
	}

	commands.InsertDiagnosticMarker(label, color);
}

void FrameExecutionDiagnostics::InsertGpuMarker(
	CommandContext& commands,
	const Engine::Diagnostics::DiagnosticName& name,
	RhiDiagnosticLabelColor color) const noexcept
{
	InsertGpuMarker(commands, name.GetCanonicalName(), color);
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
		m_resolvedTimers.push_back(ResolvedGpuTiming{
		    .Label = record.Label,
		    .BeginTicks = beginTicks,
		    .EndTicks = endTicks,
		    .DurationTicks = durationTicks,
		    .DurationMilliseconds = durationMilliseconds,
		    .Depth = record.Depth});
	}

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

bool FrameExecutionDiagnostics::WriteTimestamp(CommandContext& commands, RhiTimestampQueryHandle query) noexcept
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