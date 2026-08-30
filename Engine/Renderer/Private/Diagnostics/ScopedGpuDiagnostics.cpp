#include "PCH.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"

#include "Commands/RenderCommandContext.h"

#include <utility>

ScopedGpuEvent::ScopedGpuEvent(RenderCommandContext& commands, std::string label, RhiDiagnosticLabelColor color) noexcept :
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

bool ScopedGpuEvent::IsActive() const noexcept
{
	return m_commands != nullptr;
}

ScopedGpuEvent::ScopedGpuEvent(ScopedGpuEvent&& other) noexcept :
    m_commands(other.m_commands)
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
    RenderCommandContext& commands,
    std::string label,
    RhiTimestampQueryHandle beginQuery,
    RhiTimestampQueryHandle endQuery,
    ERhiQueueType queueType) noexcept :
    m_owner(&owner),
    m_label(std::move(label)),
    m_beginQuery(beginQuery),
    m_endQuery(endQuery),
    m_queueType(queueType)
{
	if (m_label.empty() || !m_beginQuery || !m_endQuery || !m_owner->WriteTimestamp(commands, m_beginQuery))
	{
		Reset();
		return;
	}

	m_commands = &commands;
	m_depth = commands.AcquireGpuDiagnosticScopeDepth();
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

	m_owner->RecordCompletedTimer(std::move(m_label), m_beginQuery, m_endQuery, m_queueType, m_depth);
	m_commands->ReleaseGpuDiagnosticScopeDepth();
	m_owner = nullptr;
	m_commands = nullptr;
	m_beginQuery = {};
	m_endQuery = {};
}

bool ScopedGpuTimer::IsActive() const noexcept
{
	return m_owner != nullptr;
}

ScopedGpuTimer::ScopedGpuTimer(ScopedGpuTimer&& other) noexcept :
    m_owner(other.m_owner),
    m_commands(other.m_commands),
    m_label(std::move(other.m_label)),
    m_beginQuery(other.m_beginQuery),
    m_endQuery(other.m_endQuery),
    m_queueType(other.m_queueType),
    m_depth(other.m_depth)
{
	other.m_owner = nullptr;
	other.m_commands = nullptr;
	other.m_beginQuery = {};
	other.m_endQuery = {};
	other.m_queueType = ERhiQueueType::Graphics;
	other.m_depth = 0;
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
		m_queueType = other.m_queueType;
		m_depth = other.m_depth;
		other.m_owner = nullptr;
		other.m_commands = nullptr;
		other.m_beginQuery = {};
		other.m_endQuery = {};
		other.m_queueType = ERhiQueueType::Graphics;
		other.m_depth = 0;
	}

	return *this;
}

void ScopedGpuTimer::Reset() noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->ReleaseTimestampQuery(m_beginQuery);
		m_owner->ReleaseTimestampQuery(m_endQuery);
		if (m_commands != nullptr)
		{
			m_commands->ReleaseGpuDiagnosticScopeDepth();
		}
	}

	m_owner = nullptr;
	m_commands = nullptr;
	m_label.clear();
	m_beginQuery = {};
	m_endQuery = {};
	m_queueType = ERhiQueueType::Graphics;
	m_depth = 0;
}

ScopedGpuScope::ScopedGpuScope(ScopedGpuEvent eventScope, ScopedGpuTimer timerScope) noexcept :
    m_eventScope(std::move(eventScope)),
    m_timerScope(std::move(timerScope))
{
}

bool ScopedGpuScope::IsActive() const noexcept
{
	return m_eventScope.IsActive() || m_timerScope.IsActive();
}

ScopedGpuScope::ScopedGpuScope(ScopedGpuScope&& other) noexcept :
    m_eventScope(std::move(other.m_eventScope)),
    m_timerScope(std::move(other.m_timerScope))
{
}

ScopedGpuScope& ScopedGpuScope::operator=(ScopedGpuScope&& other) noexcept
{
	if (this != &other)
	{
		m_eventScope = std::move(other.m_eventScope);
		m_timerScope = std::move(other.m_timerScope);
	}

	return *this;
}
