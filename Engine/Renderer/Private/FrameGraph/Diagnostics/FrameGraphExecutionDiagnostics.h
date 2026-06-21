#pragma once

#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <string_view>

class PassExecutionDiagnostics;
struct FrameGraphPassNode;
class RenderCommandContext;

class FrameGraphExecutionDiagnostics final
{
  public:
	FrameGraphExecutionDiagnostics(FrameExecutionDiagnostics& frameDiagnostics, RenderCommandContext& commands) noexcept;

	RendererDiagnosticMarkerVerbosity GetMarkerVerbosity() const noexcept { return m_markerVerbosity; }
	bool ShouldEmitFramePassMarkers() const noexcept;
	bool ShouldEmitDetailedMarkers() const noexcept;

	ScopedGpuScope BeginPassScope(PassExecutionDiagnostics& passDiagnostics) const noexcept;
	void InsertPassAliasingBarrierMarker(const FrameGraphPassNode& passRecord) const;
	void InsertPassResourceBarrierMarker(const FrameGraphPassNode& passRecord) const;
	void InsertFrameEndAliasingBarrierMarker() const;
	void InsertFrameEndResourceBarrierMarker() const;

  private:
	void InsertDetailedMarker(std::string_view label) const;

	FrameExecutionDiagnostics* m_frameDiagnostics = nullptr;
	RenderCommandContext* m_commands = nullptr;
	RendererDiagnosticMarkerVerbosity m_markerVerbosity = RendererDiagnosticMarkerVerbosity::FramePass;
};
