#pragma once

#include "Validation/RhiSmokeFrameControl.h"

#include <string_view>

class RuntimeApplication;

struct RhiSmokeSessionConfig final
{
	bool Enabled = false;
	bool TraceLogging = false;
	RhiSmokeFrameControlConfig FrameControl;
};

struct RhiSmokeSessionState final
{
	bool DiagnosticsLogged = false;
	bool RendererEvidenceLogged = false;
	RhiSmokeFrameControlState FrameControl;
};

namespace RhiSmokeSession
{
	RhiSmokeSessionConfig LoadConfig() noexcept;
	void ApplyLoggingConfig(const RhiSmokeSessionConfig& config) noexcept;
	void LogDiagnosticsCapabilities(const RhiSmokeSessionConfig& config, RuntimeApplication& app, RhiSmokeSessionState& state) noexcept;
	void LogRendererEvidence(
	    const RhiSmokeSessionConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeSessionState& state,
	    std::string_view evidenceLabel,
	    std::string_view validationLabel) noexcept;
	void InitializeFrameControl(const RhiSmokeSessionConfig& config, RuntimeApplication& app, RhiSmokeSessionState& state) noexcept;
}
