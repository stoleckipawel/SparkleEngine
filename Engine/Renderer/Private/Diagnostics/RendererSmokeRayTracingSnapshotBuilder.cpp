#include "PCH.h"

#include "Diagnostics/RendererSmokeRayTracingSnapshotBuilder.h"

#include "RayTracing/RayTracingPerformanceMetrics.h"
#include "RayTracing/RenderRayTracingScene.h"
#include "RayTracing/RTIndirectSpecularRuntimeDiagnostics.h"

namespace RendererSmokeRayTracingSnapshotBuilderDetails
{
	RendererSmokeRayTracingBlasDiagnostics BuildBlasDiagnostics(const RayTracingPerformanceMetrics& metrics) noexcept
	{
		return RendererSmokeRayTracingBlasDiagnostics{
		    .ReferencedMeshCount = metrics.Blas.ReferencedMeshCount,
		    .BuiltCount = metrics.Blas.BuiltCount,
		    .ReusedCount = metrics.Blas.ReusedCount,
		    .CpuMilliseconds = metrics.Blas.CpuMilliseconds,
		    .GpuMilliseconds = metrics.Blas.GpuMilliseconds};
	}

	RendererSmokeRayTracingClassicTlasDiagnostics BuildClassicTlasDiagnostics(
	    const RenderRayTracingScene& rayTracingScene,
	    const RayTracingPerformanceMetrics& metrics) noexcept
	{
		return RendererSmokeRayTracingClassicTlasDiagnostics{
		    .Valid = rayTracingScene.HasValidTlas(),
		    .InstanceCount = rayTracingScene.GetTlasInstanceCount(),
		    .CandidateInstanceCount = metrics.ClassicTlas.CandidateInstanceCount,
		    .MissingGpuMeshCount = metrics.ClassicTlas.MissingGpuMeshCount,
		    .RejectedBlasCount = metrics.ClassicTlas.RejectedBlasCount,
		    .Built = metrics.ClassicTlas.Built,
		    .CpuMilliseconds = metrics.ClassicTlas.CpuMilliseconds,
		    .InstancePreparationCpuMilliseconds = metrics.ClassicTlas.InstancePreparationCpuMilliseconds,
		    .GpuMilliseconds = metrics.ClassicTlas.GpuMilliseconds};
	}

	RendererSmokeRayTracingPtlasPlannerDiagnostics BuildPtlasPlannerDiagnostics(
	    const RhiRayTracingCapabilities& capabilities,
	    const RayTracingPerformanceMetrics& metrics) noexcept
	{
		return RendererSmokeRayTracingPtlasPlannerDiagnostics{
		    .Provider = capabilities.Groups.PartitionedTlas.Provider,
		    .Supported = capabilities.Groups.PartitionedTlas.Supported,
		    .TotalRenderInstanceCount = metrics.PtlasPlanner.TotalRenderInstanceCount,
		    .TraceableInstanceCount = metrics.PtlasPlanner.TraceableInstanceCount,
		    .StaticTraceableInstanceCount = metrics.PtlasPlanner.StaticTraceableInstanceCount,
		    .DynamicTraceableInstanceCount = metrics.PtlasPlanner.DynamicTraceableInstanceCount,
		    .PartitionsPerAxis = metrics.PtlasPlanner.PartitionsPerAxis,
		    .PartitionCount = metrics.PtlasPlanner.PartitionCount,
		    .GridPartitionCount = metrics.PtlasPlanner.GridPartitionCount,
		    .DirtyTransformCount = metrics.PtlasPlanner.DirtyTransformCount,
		    .MovedPartitionCount = metrics.PtlasPlanner.MovedPartitionCount,
		    .GlobalPartitionEligibleCount = metrics.PtlasPlanner.GlobalPartitionEligibleCount,
		    .GlobalPartitionInstanceCount = metrics.PtlasPlanner.GlobalPartitionInstanceCount,
		    .ActivePartitionCount = metrics.PtlasPlanner.ActivePartitionCount,
		    .MaxPartitionActivityCount = metrics.PtlasPlanner.MaxPartitionActivityCount,
		    .DuplicateStableIndexCount = metrics.PtlasPlanner.DuplicateStableIndexCount,
		    .Overflow = metrics.PtlasPlanner.Overflow};
	}

	RendererSmokeRayTracingPtlasGpuUpdateDiagnostics BuildPtlasGpuUpdateDiagnostics(
	    const RayTracingPerformanceMetrics& metrics) noexcept
	{
		return RendererSmokeRayTracingPtlasGpuUpdateDiagnostics{
		    .RequestedWriterPath = metrics.PtlasGpuUpdates.RequestedWriterPath,
		    .SelectedWriterPath = metrics.PtlasGpuUpdates.SelectedWriterPath,
		    .WriterSelectionReason = metrics.PtlasGpuUpdates.WriterSelectionReason,
		    .LogicalUpdateCount = metrics.PtlasGpuUpdates.LogicalUpdateCount,
		    .NativeOperationCount = metrics.PtlasGpuUpdates.NativeOperationCount,
		    .ValidationMismatchCount = metrics.PtlasGpuUpdates.ValidationMismatchCount,
		    .GpuDrivenOperationApiSupported = metrics.PtlasGpuUpdates.GpuDrivenOperationApiSupported,
		    .GpuLogicalUpdateWriterAvailable = metrics.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable,
		    .FullGpuNativePackAvailable = metrics.PtlasGpuUpdates.FullGpuNativePackAvailable,
		    .FullGpuNativePackSubmitted = metrics.PtlasGpuUpdates.FullGpuNativePackSubmitted,
		    .CpuPackMilliseconds = metrics.PtlasGpuUpdates.CpuPackMilliseconds,
		    .GpuDirtyDetectionMilliseconds = metrics.PtlasGpuUpdates.GpuDirtyDetectionMilliseconds,
		    .GpuNativePackMilliseconds = metrics.PtlasGpuUpdates.GpuNativePackMilliseconds,
		    .PtlasUpdateGpuMilliseconds = metrics.PtlasGpuUpdates.PtlasUpdateGpuMilliseconds};
	}

	RendererSmokeRTIndirectSpecularDiagnostics BuildRTIndirectSpecularDiagnostics() noexcept
	{
		const RTIndirectSpecularRuntimeDiagnosticsSnapshot snapshot = RTIndirectSpecularRuntimeDiagnostics::Capture();
		return RendererSmokeRTIndirectSpecularDiagnostics{
		    .StatusReason = snapshot.StatusReason,
		    .Enabled = snapshot.Enabled,
		    .SampleMode = static_cast<std::uint32_t>(snapshot.SampleMode),
		    .DebugMode = static_cast<std::uint32_t>(snapshot.DebugMode),
		    .MaxDistance = snapshot.MaxDistance,
		    .HitDataAvailable = snapshot.HitDataAvailable,
		    .HitInstanceCount = snapshot.HitInstanceCount,
		    .HitMaterialCount = snapshot.HitMaterialCount,
		    .GpuTimingLabel = "RT Indirect Specular Ray Query"};
	}
}

RendererSmokeRayTracingDiagnostics RendererSmokeRayTracingSnapshotBuilder::Build(
    const RhiRayTracingCapabilities& capabilities,
    const RenderRayTracingScene* rayTracingScene) noexcept
{
	RendererSmokeRayTracingDiagnostics diagnostics{};
	diagnostics.Capability.Supported = capabilities.SupportsRayTracing;
	diagnostics.Capability.InlineRayQuerySupported = capabilities.SupportsInlineRayQuery;
	diagnostics.RTIndirectSpecular = RendererSmokeRayTracingSnapshotBuilderDetails::BuildRTIndirectSpecularDiagnostics();

	if (rayTracingScene == nullptr)
	{
		return diagnostics;
	}

	const RayTracingPerformanceMetrics& metrics = rayTracingScene->GetPerformanceMetrics();
	diagnostics.Capability.TopLevelProvider = metrics.Providers.TopLevelProvider;
	diagnostics.Capability.TopLevelProviderReason = metrics.Providers.TopLevelProviderReason;
	diagnostics.Capability.PartitionedTlasCapabilityReason = metrics.Providers.PartitionedTlasCapabilityReason;
	diagnostics.FrameTimings.ScenePrepareCpuMilliseconds = metrics.Timings.ScenePrepareCpuMilliseconds;
	diagnostics.FrameTimings.SceneBuildCpuMilliseconds = metrics.Timings.SceneBuildCpuMilliseconds;
	diagnostics.FrameTimings.RayTracingPassGpuMilliseconds = metrics.Timings.RayTracingPassGpuMilliseconds;
	diagnostics.FrameTimings.RTIndirectSpecularGpuMilliseconds = metrics.Timings.RTIndirectSpecularGpuMilliseconds;
	diagnostics.Blas = RendererSmokeRayTracingSnapshotBuilderDetails::BuildBlasDiagnostics(metrics);
	diagnostics.ClassicTlas = RendererSmokeRayTracingSnapshotBuilderDetails::BuildClassicTlasDiagnostics(*rayTracingScene, metrics);
	diagnostics.PtlasPlanner = RendererSmokeRayTracingSnapshotBuilderDetails::BuildPtlasPlannerDiagnostics(capabilities, metrics);
	diagnostics.PtlasGpuUpdates = RendererSmokeRayTracingSnapshotBuilderDetails::BuildPtlasGpuUpdateDiagnostics(metrics);
	return diagnostics;
}
