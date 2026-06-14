#include "PCH.h"

#include "Validation/RhiSmokeRayTracingEvidence.h"

#include "Renderer/Public/Diagnostics/RendererSmokeRayTracingDiagnostics.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

namespace RhiSmokeRayTracingEvidence
{
	void Log(
	    const RendererSmokeRayTracingDiagnostics& diagnostics,
	    std::string_view evidenceLabel,
	    const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		SPDLOG_LOGGER_INFO(
		    logger,
		    "{} rayTracing: supported={} inlineRayQuery={} topLevelProvider={} sceneCpuMs(prepare={:.3f}, build={:.3f}) "
		    "rayTracingPassGpuMs={:.3f}",
		    evidenceLabel,
		    diagnostics.Capability.Supported,
		    diagnostics.Capability.InlineRayQuerySupported,
		    RhiRayTracingTopLevelProviderToString(diagnostics.Capability.TopLevelProvider),
		    diagnostics.FrameTimings.ScenePrepareCpuMilliseconds,
		    diagnostics.FrameTimings.SceneBuildCpuMilliseconds,
		    diagnostics.FrameTimings.RayTracingPassGpuMilliseconds);

		SPDLOG_LOGGER_INFO(
		    logger,
		    "{} rayTracing.blas: referencedMeshes={} built={} reused={} cpuMs={:.3f} gpuMs={:.3f}",
		    evidenceLabel,
		    diagnostics.Blas.ReferencedMeshCount,
		    diagnostics.Blas.BuiltCount,
		    diagnostics.Blas.ReusedCount,
		    diagnostics.Blas.CpuMilliseconds,
		    diagnostics.Blas.GpuMilliseconds);

		SPDLOG_LOGGER_INFO(
		    logger,
		    "{} rayTracing.classicTlas: valid={} instances={} candidates={} missingGpuMesh={} rejectedBlas={} built={} cpuMs={:.3f} "
		    "instancePrepCpuMs={:.3f} gpuMs={:.3f}",
		    evidenceLabel,
		    diagnostics.ClassicTlas.Valid,
		    diagnostics.ClassicTlas.InstanceCount,
		    diagnostics.ClassicTlas.CandidateInstanceCount,
		    diagnostics.ClassicTlas.MissingGpuMeshCount,
		    diagnostics.ClassicTlas.RejectedBlasCount,
		    diagnostics.ClassicTlas.Built,
		    diagnostics.ClassicTlas.CpuMilliseconds,
		    diagnostics.ClassicTlas.InstancePreparationCpuMilliseconds,
		    diagnostics.ClassicTlas.GpuMilliseconds);

		SPDLOG_LOGGER_INFO(
		    logger,
		    "{} rayTracing.ptlasPlanner: provider={} supported={} partitions={} dirtyTransforms={} movedPartitions={} "
		    "globalPartitionInstances={} duplicateStableIndices={} overflow={}",
		    evidenceLabel,
		    RhiPartitionedTlasProviderToString(diagnostics.PtlasPlanner.Provider),
		    diagnostics.PtlasPlanner.Supported,
		    diagnostics.PtlasPlanner.PartitionCount,
		    diagnostics.PtlasPlanner.DirtyTransformCount,
		    diagnostics.PtlasPlanner.MovedPartitionCount,
		    diagnostics.PtlasPlanner.GlobalPartitionInstanceCount,
		    diagnostics.PtlasPlanner.DuplicateStableIndexCount,
		    diagnostics.PtlasPlanner.Overflow);

		SPDLOG_LOGGER_INFO(
		    logger,
		    "{} rayTracing.ptlasGpuUpdates: requestedWriterPath={} selectedWriterPath={} reason={} logicalUpdates={} "
		    "nativeOperations={} validationMismatches={} "
		    "fullGpuNativePackSupported={} fullGpuNativePackSubmitted={} cpuPackMs={:.3f} gpuDirtyMs={:.3f} "
		    "gpuNativePackMs={:.3f} ptlasUpdateGpuMs={:.3f}",
		    evidenceLabel,
		    RhiPartitionedTlasOperationWriterPathToString(diagnostics.PtlasGpuUpdates.RequestedWriterPath),
		    RhiPartitionedTlasOperationWriterPathToString(diagnostics.PtlasGpuUpdates.SelectedWriterPath),
		    diagnostics.PtlasGpuUpdates.WriterSelectionReason,
		    diagnostics.PtlasGpuUpdates.LogicalUpdateCount,
		    diagnostics.PtlasGpuUpdates.NativeOperationCount,
		    diagnostics.PtlasGpuUpdates.ValidationMismatchCount,
		    diagnostics.PtlasGpuUpdates.FullGpuNativePackSupported,
		    diagnostics.PtlasGpuUpdates.FullGpuNativePackSubmitted,
		    diagnostics.PtlasGpuUpdates.CpuPackMilliseconds,
		    diagnostics.PtlasGpuUpdates.GpuDirtyDetectionMilliseconds,
		    diagnostics.PtlasGpuUpdates.GpuNativePackMilliseconds,
		    diagnostics.PtlasGpuUpdates.PtlasUpdateGpuMilliseconds);
	}

	bool Validate(
	    const RendererSmokeRayTracingDiagnostics& diagnostics,
	    std::string_view validationLabel,
	    const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		if (!diagnostics.PtlasPlanner.Overflow && diagnostics.PtlasPlanner.DuplicateStableIndexCount == 0)
		{
			return true;
		}

		SPDLOG_LOGGER_ERROR(
		    logger,
		    "{}: ray tracing PTLAS planner reported overflow={} duplicateStableIndices={}.",
		    validationLabel,
		    diagnostics.PtlasPlanner.Overflow,
		    diagnostics.PtlasPlanner.DuplicateStableIndexCount);
		return false;
	}
}
