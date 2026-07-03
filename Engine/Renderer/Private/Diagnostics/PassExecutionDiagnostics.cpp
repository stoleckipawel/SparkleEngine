#include "PCH.h"

#include "Diagnostics/PassExecutionDiagnostics.h"

#include "Renderer/Public/Debug/RendererCVars.h"

PassExecutionDiagnostics::PassExecutionDiagnostics(
    FrameExecutionDiagnostics& frameDiagnostics,
    RenderCommandContext& commands,
    std::string_view passScopeLabel,
    EFrameGraphPassFlags passKind) noexcept :
    m_frameDiagnostics(&frameDiagnostics),
    m_commands(&commands),
    m_passScopeLabel(passScopeLabel),
    m_passColor(GetPassEventColor(passKind))
{
}

ScopedGpuScope PassExecutionDiagnostics::BeginPassGpuScope() noexcept
{
	if (m_frameDiagnostics == nullptr || m_commands == nullptr)
	{
		return {};
	}

	return m_frameDiagnostics->BeginGpuScope(*m_commands, m_passScopeLabel, m_passColor);
}

ScopedGpuScope PassExecutionDiagnostics::BeginGpuScope(std::string_view label) noexcept
{
	if (m_frameDiagnostics == nullptr || m_commands == nullptr ||
	    CVarRendererDiagnosticMarkerVerbosity.Get() != RendererDiagnosticMarkerVerbosity::Detailed)
	{
		return {};
	}

	const std::string eventLabel = FormatEventScopeLabel(label);
	return m_frameDiagnostics->BeginGpuScope(*m_commands, eventLabel, m_passColor);
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
		case EFrameGraphPassFlags::ExternalProvider:
			return RhiDiagnosticLabelColor{.Red = 186, .Green = 128, .Blue = 255, .Alpha = 255};
		default:
			return RhiDiagnosticLabelColor{.Red = 224, .Green = 224, .Blue = 224, .Alpha = 255};
	}
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
