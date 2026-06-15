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

	const char* GetCurrentViewModeSummary(RenderViewMode viewMode) noexcept
	{
		switch (viewMode)
		{
			case RenderViewMode::RayTracingPartitions:
				return "Each mesh is colored by its logical PTLAS partition.";
			case RenderViewMode::RayTracingPartitionUpdates:
				return "Yellow marks dirty transforms; orange/magenta mark partition movement.";
			case RenderViewMode::RayTracingInstanceMovement:
				return "Cyan marks transform updates; red marks partition crossings.";
			case RenderViewMode::RayTracingGpuDrivenUpdates:
				return "Shows whether the requested update writer can use GPU-side paths.";
			case RenderViewMode::RayTracingTopLevelMode:
				return "Blue means partitioned TLAS is selected; classic TLAS should not dominate.";
			case RenderViewMode::RayTracingNativeOperations:
				return "Bright instances are the operations submitted for PTLAS update.";
			case RenderViewMode::RayTracingProviderStatus:
				return "Provider/partition status view for backend support and fallback checks.";
			case RenderViewMode::Lit:
			case RenderViewMode::Wireframe:
			case RenderViewMode::GBufferDiffuse:
			case RenderViewMode::GBufferNormal:
			case RenderViewMode::GBufferRoughness:
			case RenderViewMode::GBufferMetallic:
			case RenderViewMode::GBufferEmissive:
			case RenderViewMode::GBufferAmbientOcclusion:
			case RenderViewMode::GBufferSubsurfaceColor:
			case RenderViewMode::GBufferSubsurfaceStrength:
			case RenderViewMode::DirectDiffuse:
			case RenderViewMode::DirectSpecular:
			case RenderViewMode::DirectSubsurface:
			case RenderViewMode::IndirectDiffuse:
			case RenderViewMode::IndirectSpecular:
			case RenderViewMode::IndirectSubsurface:
			case RenderViewMode::InstanceGroups:
			case RenderViewMode::Count:
				break;
		}

		return "";
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
	const RenderViewMode currentViewMode = CVarRenderViewMode.Get();
	ImGui::TextWrapped("%s", ViewportRayTracingDebugOverlayRows::GetCurrentViewModeSummary(currentViewMode));
	const bool hasLiveUpdates =
	    rayTracing.PtlasPlanner.DirtyTransformCount > 0 || rayTracing.PtlasPlanner.MovedPartitionCount > 0 ||
	    rayTracing.PtlasGpuUpdates.LogicalUpdateCount > 0 || rayTracing.PtlasGpuUpdates.NativeOperationCount > 0;
	ViewportRayTracingDebugOverlayRows::DrawMetricRow("Demo workload", hasLiveUpdates ? "live transform updates" : "static scene / waiting");
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
