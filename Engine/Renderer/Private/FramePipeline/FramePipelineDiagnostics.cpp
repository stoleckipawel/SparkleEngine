#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Core/Public/Diagnostics/LiveProfiler.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "FrameGraph/FrameGraph.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

std::uint32_t FramePipeline::GetLastUnresolvedBarrierWarningCount() const noexcept
{
	return m_frameGraph != nullptr ? m_frameGraph->GetLastUnresolvedBarrierWarningCount() : 0u;
}

std::uint32_t FramePipeline::GetLastMissingExecutionBindingCount() const noexcept
{
	return m_frameGraph != nullptr ? m_frameGraph->GetLastMissingExecutionBindingCount() : 0u;
}

std::uint32_t FramePipeline::GetCompiledTransientResourceCount() const noexcept
{
	return m_frameGraph != nullptr ? m_frameGraph->GetCompiledTransientResourceCount() : 0u;
}

std::uint32_t FramePipeline::GetCompiledImportedResourceCount() const noexcept
{
	return m_frameGraph != nullptr ? m_frameGraph->GetCompiledImportedResourceCount() : 0u;
}

std::uint32_t FramePipeline::GetCompiledPersistentResourceCount() const noexcept
{
	return m_frameGraph != nullptr ? m_frameGraph->GetCompiledPersistentResourceCount() : 0u;
}

std::uint32_t FramePipeline::GetAvailableViewportProductCount() const noexcept
{
	std::uint32_t count = 0;
	count += m_viewportRenderProducts.HasOutput(RenderOutputFlags::SceneColor) ? 1u : 0u;
	count += m_viewportRenderProducts.HasOutput(RenderOutputFlags::SceneDepth) ? 1u : 0u;
	count += m_viewportRenderProducts.HasOutput(RenderOutputFlags::Normals) ? 1u : 0u;
	count += m_viewportRenderProducts.HasOutput(RenderOutputFlags::ObjectId) ? 1u : 0u;
	count += m_viewportRenderProducts.HasOutput(RenderOutputFlags::OverlayMask) ? 1u : 0u;
	return count;
}

RendererFrameTimingDiagnosticsSnapshot FramePipeline::CaptureFrameTimingDiagnosticsSnapshot() const
{
	const FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();
	RendererFrameTimingDiagnosticsSnapshot snapshot;
	snapshot.GpuTimingStatus =
	    frameDiagnostics.IsGpuTimingAvailable() ? ERendererDiagnosticStatus::Available : ERendererDiagnosticStatus::Unsupported;
	const std::vector<ResolvedGpuTiming>& resolvedTimings = frameDiagnostics.GetResolvedTimings();
	snapshot.GpuTimings.reserve(resolvedTimings.size());
	for (const ResolvedGpuTiming& resolvedTiming : resolvedTimings)
	{
		snapshot.GpuTimings.push_back(
		    RendererGpuTimingMetric{
		        .Label = resolvedTiming.Label,
		        .BeginTicks = resolvedTiming.BeginTicks,
		        .EndTicks = resolvedTiming.EndTicks,
		        .DurationTicks = resolvedTiming.DurationTicks,
		        .DurationMilliseconds = resolvedTiming.DurationMilliseconds,
		        .Depth = resolvedTiming.Depth});
	}
	snapshot.CpuFrameTimingStatus = ERendererDiagnosticStatus::Available;
	snapshot.CpuFrameTimingReason = "CPU frame scopes are owned by Core LiveProfiler; renderer snapshot keeps GPU frame timings here.";
	return snapshot;
}

FrameExecutionDiagnostics& FramePipeline::GetCurrentFrameDiagnostics() noexcept
{
	return *m_frameExecutionDiagnostics[m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex()];
}

const FrameExecutionDiagnostics& FramePipeline::GetCurrentFrameDiagnostics() const noexcept
{
	return *m_frameExecutionDiagnostics[m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex()];
}

void FramePipeline::ReportResolvedTimings(std::uint32_t frameIndex, const FrameExecutionDiagnostics& frameDiagnostics) const noexcept
{
	const auto& resolvedTimers = frameDiagnostics.GetResolvedTimings();

	PublishLiveGpuTimings(resolvedTimers);

	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");

	if (rendererLogger == nullptr || !rendererLogger->should_log(spdlog::level::trace))
	{
		return;
	}

	if (resolvedTimers.empty())
	{
		return;
	}

	SPDLOG_LOGGER_TRACE(rendererLogger, "Resolved GPU timings for frame slot {} ({} scopes)", frameIndex, resolvedTimers.size());
	for (const ResolvedGpuTiming& resolvedTimer : resolvedTimers)
	{
		SPDLOG_LOGGER_TRACE(
		    rendererLogger,
		    "  {}: {:.3f} ms ({} ticks)",
		    resolvedTimer.Label,
		    resolvedTimer.DurationMilliseconds,
		    resolvedTimer.DurationTicks);
	}
}

void FramePipeline::PublishLiveGpuTimings(const std::vector<ResolvedGpuTiming>& resolvedTimers) const noexcept
{
	if (resolvedTimers.empty())
	{
		return;
	}

	Diagnostics::LiveProfiler& profiler = Diagnostics::LiveProfiler::Get();
	if (!profiler.IsEnabled())
	{
		return;
	}

	std::vector<Diagnostics::LiveProfiler::GpuTimingEntry> entries;
	entries.reserve(resolvedTimers.size());
	for (const ResolvedGpuTiming& resolvedTimer : resolvedTimers)
	{
		entries.push_back(
		    Diagnostics::LiveProfiler::GpuTimingEntry{
		        .Label = std::string_view(resolvedTimer.Label),
		        .DurationMicroseconds = static_cast<std::uint64_t>(resolvedTimer.DurationMilliseconds * 1000.0),
		        .Depth = resolvedTimer.Depth});
	}

	profiler.SubmitGpuFrame(entries.data(), entries.size());
}
