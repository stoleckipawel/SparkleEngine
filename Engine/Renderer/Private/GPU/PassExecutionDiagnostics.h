#pragma once

#include "GPU/FrameExecutionDiagnostics.h"

#include "Renderer/Public/FrameGraph/FrameGraphPassFlags.h"

#include "Core/Public/Diagnostics/Trace.h"

#include <string>
#include <string_view>

class PassExecutionDiagnostics final
{
  public:
	PassExecutionDiagnostics(
	    FrameExecutionDiagnostics& frameDiagnostics,
	    CommandContext& commands,
	    std::string_view diagnosticName,
	    std::string_view passDisplayLabel,
	    std::string_view passScopeLabel,
	    EFrameGraphPassFlags passKind) noexcept;

	bool SupportsGpuEvents() const noexcept;
	bool SupportsTimestampQueries() const noexcept;

	ScopedGpuEvent BeginPassGpuEvent() noexcept;
	ScopedGpuTimer BeginPassTimer() noexcept;
	ScopedGpuEvent BeginGpuEvent(std::string_view label) noexcept;
	ScopedGpuEvent BeginGpuEvent(const Engine::Diagnostics::DiagnosticName& name) noexcept;
	ScopedGpuTimer BeginTimer(std::string_view label) noexcept;
	ScopedGpuTimer BeginTimer(const Engine::Diagnostics::DiagnosticName& name) noexcept;
	void InsertGpuMarker(std::string_view label) const noexcept;
	void InsertGpuMarker(const Engine::Diagnostics::DiagnosticName& name) const noexcept;

  private:
	static RhiDiagnosticLabelColor GetPassEventColor(EFrameGraphPassFlags passKind) noexcept;
	std::string FormatDisplayLabel(std::string_view label) const;
	std::string FormatEventScopeLabel(std::string_view label) const;

	FrameExecutionDiagnostics* m_frameDiagnostics = nullptr;
	CommandContext* m_commands = nullptr;
	std::string m_diagnosticName;
	std::string m_passDisplayLabel;
	std::string m_passScopeLabel;
	RhiDiagnosticLabelColor m_passColor = {};
};

#ifndef SPARKLE_GPU_PASS_SCOPE
#define SPARKLE_GPU_PASS_SCOPE(diagnostics, nameStr)                                                         \
	const auto SPARKLE_PP_CONCAT(_gpuPassName_, __LINE__) = SPARKLE_DIAGNOSTIC_NAME(nameStr);                  \
	SPARKLE_CPU_SCOPE(SPARKLE_PP_CONCAT(_gpuPassName_, __LINE__));                                            \
	auto SPARKLE_PP_CONCAT(_gpuPassScope_, __LINE__) = (diagnostics).BeginGpuEvent(                      \
	    SPARKLE_PP_CONCAT(_gpuPassName_, __LINE__))
#endif
