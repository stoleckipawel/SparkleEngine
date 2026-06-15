#include "PCH.h"

#include "Panels/ViewportRayTracingDebugOverlay.h"

#include "Renderer/Public/Debug/RendererCVars.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdio>

namespace ViewportRayTracingDebugOverlayRows
{
	const char* YesNo(bool value) noexcept
	{
		return value ? "yes" : "no";
	}

	bool IsRayTracingDebugViewMode(RenderViewMode viewMode) noexcept
	{
		return viewMode == RenderViewMode::RayTracingPartitions || viewMode == RenderViewMode::RayTracingPartitionUpdates ||
		       viewMode == RenderViewMode::RayTracingInstanceMovement || viewMode == RenderViewMode::RayTracingGpuDrivenUpdates ||
		       viewMode == RenderViewMode::RayTracingTopLevelMode || viewMode == RenderViewMode::RayTracingNativeOperations ||
		       viewMode == RenderViewMode::RayTracingProviderStatus;
	}

	void DrawMetricRow(const char* label, const char* value) noexcept
	{
		ImGui::TextDisabled("%s", label);
		ImGui::SameLine(150.0f);
		ImGui::TextUnformatted(value);
	}

	void DrawMetricRow(const char* label, std::uint32_t value) noexcept
	{
		char buffer[32] = {};
		std::snprintf(buffer, sizeof(buffer), "%u", value);
		DrawMetricRow(label, buffer);
	}

	void DrawMetricRow(const char* label, double value) noexcept
	{
		char buffer[32] = {};
		std::snprintf(buffer, sizeof(buffer), "%.3f ms", value);
		DrawMetricRow(label, buffer);
	}
}

void ViewportRayTracingDebugOverlay::Draw(
    const RendererSmokeDiagnosticsSnapshot& diagnostics,
    const ImVec2& viewportMin) noexcept
{
	const RendererSmokeRayTracingDiagnostics& rayTracing = diagnostics.RayTracing;
	if (!rayTracing.Capability.Supported ||
	    !ViewportRayTracingDebugOverlayRows::IsRayTracingDebugViewMode(CVarRenderViewMode.Get()))
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(viewportMin.x + 12.0f, viewportMin.y + 12.0f), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.78f);
	ImGui::Begin(
	    "Ray Tracing PTLAS Overlay",
	    nullptr,
	    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
	        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

	ImGui::TextUnformatted("Ray Tracing PTLAS");
	ImGui::Separator();
	ViewportRayTracingDebugOverlayRows::DrawMetricRow(
	    "Top level",
	    RhiRayTracingTopLevelProviderToString(rayTracing.Capability.TopLevelProvider));
	ViewportRayTracingDebugOverlayRows::DrawMetricRow(
	    "PTLAS provider",
	    RhiPartitionedTlasProviderToString(rayTracing.PtlasPlanner.Provider));
	ViewportRayTracingDebugOverlayRows::DrawMetricRow(
	    "PTLAS supported",
	    ViewportRayTracingDebugOverlayRows::YesNo(rayTracing.PtlasPlanner.Supported));
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("Partitions", rayTracing.PtlasPlanner.PartitionCount);
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("Dirty transforms", rayTracing.PtlasPlanner.DirtyTransformCount);
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("Moved partitions", rayTracing.PtlasPlanner.MovedPartitionCount);
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("Global partition", rayTracing.PtlasPlanner.GlobalPartitionInstanceCount);
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("Native ops", rayTracing.PtlasGpuUpdates.NativeOperationCount);
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("Logical updates", rayTracing.PtlasGpuUpdates.LogicalUpdateCount);
	ViewportRayTracingDebugOverlayRows::DrawMetricRow(
	    "Requested writer",
	    RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.RequestedWriterPath));
	ViewportRayTracingDebugOverlayRows::DrawMetricRow(
	    "Writer path",
	    RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.SelectedWriterPath));
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("Writer reason", rayTracing.PtlasGpuUpdates.WriterSelectionReason);
	ViewportRayTracingDebugOverlayRows::DrawMetricRow(
	    "GPU op API",
	    ViewportRayTracingDebugOverlayRows::YesNo(rayTracing.PtlasGpuUpdates.GpuDrivenOperationApiSupported));
	ViewportRayTracingDebugOverlayRows::DrawMetricRow(
	    "GPU logical writer",
	    ViewportRayTracingDebugOverlayRows::YesNo(rayTracing.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable));
	ViewportRayTracingDebugOverlayRows::DrawMetricRow(
	    "GPU native writer",
	    ViewportRayTracingDebugOverlayRows::YesNo(rayTracing.PtlasGpuUpdates.FullGpuNativePackAvailable));
	ViewportRayTracingDebugOverlayRows::DrawMetricRow(
	    "GPU pack used",
	    ViewportRayTracingDebugOverlayRows::YesNo(rayTracing.PtlasGpuUpdates.FullGpuNativePackSubmitted));
	ImGui::Separator();
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("BLAS GPU", rayTracing.Blas.GpuMilliseconds);
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("Classic TLAS GPU", rayTracing.ClassicTlas.GpuMilliseconds);
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("RT pass GPU", rayTracing.FrameTimings.RayTracingPassGpuMilliseconds);
	ImGui::End();
}
