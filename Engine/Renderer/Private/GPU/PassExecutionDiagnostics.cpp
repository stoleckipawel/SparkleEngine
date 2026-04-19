#include "PCH.h"

#include "GPU/PassExecutionDiagnostics.h"

PassExecutionDiagnostics::PassExecutionDiagnostics(
	FrameExecutionDiagnostics& frameDiagnostics,
	CommandContext& commands,
	std::string_view diagnosticName,
	std::string_view passDisplayLabel,
	std::string_view passScopeLabel,
	EFrameGraphPassFlags passKind) noexcept :
	m_frameDiagnostics(&frameDiagnostics),
	m_commands(&commands),
	m_diagnosticName(diagnosticName),
	m_passDisplayLabel(passDisplayLabel),
	m_passScopeLabel(passScopeLabel),
	m_passColor(GetPassEventColor(passKind))
{
}

bool PassExecutionDiagnostics::SupportsGpuEvents() const noexcept
{
	return m_frameDiagnostics != nullptr && m_frameDiagnostics->SupportsGpuEvents();
}

bool PassExecutionDiagnostics::SupportsTimestampQueries() const noexcept
{
	return m_frameDiagnostics != nullptr && m_frameDiagnostics->SupportsTimestampQueries();
}

ScopedGpuEvent PassExecutionDiagnostics::BeginPassGpuEvent() noexcept
{
	if (m_frameDiagnostics == nullptr || m_commands == nullptr)
	{
		return {};
	}

	return m_frameDiagnostics->BeginGpuEvent(*m_commands, m_passScopeLabel, m_passColor);
}

ScopedGpuTimer PassExecutionDiagnostics::BeginPassTimer() noexcept
{
	if (m_frameDiagnostics == nullptr || m_commands == nullptr)
	{
		return {};
	}

	return m_frameDiagnostics->BeginTimer(*m_commands, m_passDisplayLabel);
}

ScopedGpuEvent PassExecutionDiagnostics::BeginGpuEvent(std::string_view label) noexcept
{
	if (m_frameDiagnostics == nullptr || m_commands == nullptr)
	{
		return {};
	}

	return m_frameDiagnostics->BeginGpuEvent(*m_commands, FormatEventScopeLabel(label), m_passColor);
}

ScopedGpuTimer PassExecutionDiagnostics::BeginTimer(std::string_view label) noexcept
{
	if (m_frameDiagnostics == nullptr || m_commands == nullptr)
	{
		return {};
	}

	return m_frameDiagnostics->BeginTimer(*m_commands, FormatDisplayLabel(label));
}

ScopedGpuEvent PassExecutionDiagnostics::BeginGpuEvent(const Engine::Diagnostics::DiagnosticName& name) noexcept
{
	return BeginGpuEvent(name.GetCanonicalName());
}

ScopedGpuTimer PassExecutionDiagnostics::BeginTimer(const Engine::Diagnostics::DiagnosticName& name) noexcept
{
	return BeginTimer(name.GetCanonicalName());
}

void PassExecutionDiagnostics::InsertGpuMarker(std::string_view label) const noexcept
{
	if (m_frameDiagnostics == nullptr || m_commands == nullptr)
	{
		return;
	}

	m_frameDiagnostics->InsertGpuMarker(*m_commands, FormatEventScopeLabel(label), m_passColor);
}

void PassExecutionDiagnostics::InsertGpuMarker(const Engine::Diagnostics::DiagnosticName& name) const noexcept
{
	InsertGpuMarker(name.GetCanonicalName());
}

RhiDiagnosticLabelColor PassExecutionDiagnostics::GetPassEventColor(EFrameGraphPassFlags passKind) noexcept
{
	switch (passKind)
	{
		case EFrameGraphPassFlags::Raster:
			return RhiDiagnosticLabelColor{.Red = 76, .Green = 148, .Blue = 255, .Alpha = 255};
		case EFrameGraphPassFlags::Compute:
			return RhiDiagnosticLabelColor{.Red = 255, .Green = 162, .Blue = 76, .Alpha = 255};
		case EFrameGraphPassFlags::Transfer:
			return RhiDiagnosticLabelColor{.Red = 115, .Green = 204, .Blue = 122, .Alpha = 255};
		default:
			return RhiDiagnosticLabelColor{.Red = 224, .Green = 224, .Blue = 224, .Alpha = 255};
	}
}

std::string PassExecutionDiagnostics::FormatDisplayLabel(std::string_view label) const
{
	if (label.empty())
	{
		return m_passDisplayLabel;
	}

	std::string displayLabel = m_passDisplayLabel;
	displayLabel += " :: ";
	displayLabel.append(label.begin(), label.end());
	return displayLabel;
}

std::string PassExecutionDiagnostics::FormatEventScopeLabel(std::string_view label) const
{
	if (label.empty())
	{
		return m_passScopeLabel;
	}

	std::string scopeLabel = m_passScopeLabel;
	scopeLabel.push_back('/');
	scopeLabel.append(label.begin(), label.end());
	return scopeLabel;
}
