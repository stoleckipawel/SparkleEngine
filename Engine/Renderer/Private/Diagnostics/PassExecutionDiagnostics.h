#pragma once

#include "Diagnostics/FrameExecutionDiagnostics.h"

#include "FrameGraph/FrameGraphPassFlags.h"

#include <string>
#include <string_view>

class PassExecutionDiagnostics final
{
  public:
	PassExecutionDiagnostics(
	    FrameExecutionDiagnostics& frameDiagnostics,
	    RenderCommandContext& commands,
	    std::string_view passScopeLabel,
	    EFrameGraphPassFlags passKind) noexcept;

	ScopedGpuScope BeginPassGpuScope() noexcept;
	ScopedGpuScope BeginGpuScope(std::string_view label) noexcept;

  private:
	static RhiDiagnosticLabelColor GetPassEventColor(EFrameGraphPassFlags passKind) noexcept;
	std::string FormatEventScopeLabel(std::string_view label) const;

	FrameExecutionDiagnostics* m_frameDiagnostics = nullptr;
	RenderCommandContext* m_commands = nullptr;
	std::string m_passScopeLabel;
	RhiDiagnosticLabelColor m_passColor = {};
};
