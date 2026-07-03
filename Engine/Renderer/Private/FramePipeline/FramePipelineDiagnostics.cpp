#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

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
	snapshot.GpuTimingAvailable = frameDiagnostics.IsGpuTimingAvailable();
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
