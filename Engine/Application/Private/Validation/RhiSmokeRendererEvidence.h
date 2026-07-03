#pragma once

#include <string_view>

class RuntimeApplication;

namespace RhiSmokeRendererEvidence
{
	void LogRhiDiagnosticsCapabilities(RuntimeApplication& app, bool& logged) noexcept;
	bool LogRendererEvidence(
	    RuntimeApplication& app,
	    bool& logged,
	    std::string_view validationLabel) noexcept;
}
