#include "PCH.h"

#include "Diagnostics/RendererSmokeRayTracingSnapshotBuilder.h"

#include "RayTracing/RayTracingPerformanceMetrics.h"
#include "RayTracing/RenderRayTracingScene.h"

namespace
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

	RendererSmokeRayTracingPtlasPlannerDiagnostics BuildPtlasPlannerDiagnostics(const RayTracingPerformanceMetrics& metrics) noexcept
	{
		return RendererSmokeRayTracingPtlasPlannerDiagnostics{
		    .Provider = metrics.Providers.PartitionedTlasProvider,
		    .Supported = metrics.Providers.SupportsPartitionedTlas,
		    .PartitionCount = metrics.PtlasPlanner.PartitionCount,
		    .DirtyTransformCount = metrics.PtlasPlanner.DirtyTransformCount,
		    .MovedPartitionCount = metrics.PtlasPlanner.MovedPartitionCount,
		    .GlobalPartitionInstanceCount = metrics.PtlasPlanner.GlobalPartitionInstanceCount,
		    .DuplicateStableIndexCount = metrics.PtlasPlanner.DuplicateStableIndexCount,
		    .Overflow = metrics.PtlasPlanner.Overflow};
	}
}

RendererSmokeRayTracingDiagnostics RendererSmokeRayTracingSnapshotBuilder::Build(
    const RhiRayTracingCapabilities& capabilities,
    const RenderRayTracingScene* rayTracingScene) noexcept
{
	RendererSmokeRayTracingDiagnostics diagnostics{};
	diagnostics.Capability.Supported = capabilities.SupportsRayTracing;
	diagnostics.Capability.InlineRayQuerySupported = capabilities.SupportsInlineRayQuery;

	if (rayTracingScene == nullptr)
	{
		return diagnostics;
	}

	const RayTracingPerformanceMetrics& metrics = rayTracingScene->GetPerformanceMetrics();
	diagnostics.Capability.TopLevelProvider = metrics.Providers.TopLevelProvider;
	diagnostics.FrameTimings.ScenePrepareCpuMilliseconds = metrics.Timings.ScenePrepareCpuMilliseconds;
	diagnostics.FrameTimings.SceneBuildCpuMilliseconds = metrics.Timings.SceneBuildCpuMilliseconds;
	diagnostics.FrameTimings.RayTracingPassGpuMilliseconds = metrics.Timings.RayTracingPassGpuMilliseconds;
	diagnostics.Blas = BuildBlasDiagnostics(metrics);
	diagnostics.ClassicTlas = BuildClassicTlasDiagnostics(*rayTracingScene, metrics);
	diagnostics.PtlasPlanner = BuildPtlasPlannerDiagnostics(metrics);
	return diagnostics;
}
