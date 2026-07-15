#include "PCH.h"

#include "FrameGraph/Diagnostics/FrameGraphExecutionDiagnostics.h"

#include "Diagnostics/PassExecutionDiagnostics.h"
#include "FrameGraph/Compiler/FrameGraphPlan.h"

#include <string>

namespace
{
	constexpr RhiDiagnosticLabelColor kFrameGraphDetailMarkerColor{.Red = 120, .Green = 160, .Blue = 220, .Alpha = 255};
}

FrameGraphExecutionDiagnostics::FrameGraphExecutionDiagnostics(
    FrameExecutionDiagnostics& frameDiagnostics,
    RenderCommandContext& commands) noexcept :
    m_frameDiagnostics(&frameDiagnostics), m_commands(&commands), m_markerVerbosity(CVarRendererDiagnosticMarkerVerbosity.Get())
{
}

bool FrameGraphExecutionDiagnostics::ShouldEmitFramePassMarkers() const noexcept
{
	return m_markerVerbosity == RendererDiagnosticMarkerVerbosity::FramePass ||
	       m_markerVerbosity == RendererDiagnosticMarkerVerbosity::Detailed;
}

RendererDiagnosticMarkerVerbosity FrameGraphExecutionDiagnostics::GetMarkerVerbosity() const noexcept
{
	return m_markerVerbosity;
}

bool FrameGraphExecutionDiagnostics::ShouldEmitDetailedMarkers() const noexcept
{
	return m_markerVerbosity == RendererDiagnosticMarkerVerbosity::Detailed;
}

ScopedGpuScope FrameGraphExecutionDiagnostics::BeginPassScope(PassExecutionDiagnostics& passDiagnostics) const noexcept
{
	return ShouldEmitFramePassMarkers() ? passDiagnostics.BeginPassGpuScope() : ScopedGpuScope{};
}

void FrameGraphExecutionDiagnostics::InsertPassAliasingBarrierMarker(const FrameGraphPassNode& passRecord) const
{
	if (passRecord.transientAliasingBarriers.empty())
	{
		return;
	}

	std::string marker = passRecord.diagnosticName;
	marker += ".AliasingBarriers";
	InsertDetailedMarker(marker);
}

void FrameGraphExecutionDiagnostics::InsertPassResourceBarrierMarker(const FrameGraphPassNode& passRecord) const
{
	if (passRecord.compiledBarriers.empty() && passRecord.compiledReleaseBarriers.empty())
	{
		return;
	}

	std::string marker = passRecord.diagnosticName;
	marker += ".ResourceBarriers";
	InsertDetailedMarker(marker);
}

void FrameGraphExecutionDiagnostics::InsertFrameBeginAliasingBarrierMarker() const
{
	InsertDetailedMarker("Renderer.FrameGraph.FrameBegin.AliasingBarriers");
}

void FrameGraphExecutionDiagnostics::InsertFrameEndResourceBarrierMarker() const
{
	InsertDetailedMarker("Renderer.FrameGraph.FrameEnd.ResourceBarriers");
}

void FrameGraphExecutionDiagnostics::InsertDetailedMarker(std::string_view label) const
{
	if (!ShouldEmitDetailedMarkers() || m_frameDiagnostics == nullptr || m_commands == nullptr || label.empty())
	{
		return;
	}

	m_frameDiagnostics->InsertGpuMarker(*m_commands, label, kFrameGraphDetailMarkerColor);
}
