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
		    "{} rayTracing: supported={} inlineRayQuery={} topLevelProvider={}",
		    evidenceLabel,
		    diagnostics.Capability.Supported,
		    diagnostics.Capability.InlineRayQuerySupported,
		    RhiRayTracingTopLevelProviderToString(diagnostics.Capability.TopLevelProvider));

		SPDLOG_LOGGER_INFO(
		    logger,
		    "{} rayTracing.blas: referencedMeshes={} built={} reused={}",
		    evidenceLabel,
		    diagnostics.Blas.ReferencedMeshCount,
		    diagnostics.Blas.BuiltCount,
		    diagnostics.Blas.ReusedCount);

		SPDLOG_LOGGER_INFO(
		    logger,
		    "{} rayTracing.classicTlas: valid={} instances={} candidates={} missingGpuMesh={} rejectedBlas={} built={}",
		    evidenceLabel,
		    diagnostics.ClassicTlas.Valid,
		    diagnostics.ClassicTlas.InstanceCount,
		    diagnostics.ClassicTlas.CandidateInstanceCount,
		    diagnostics.ClassicTlas.MissingGpuMeshCount,
		    diagnostics.ClassicTlas.RejectedBlasCount,
		    diagnostics.ClassicTlas.Built);

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
		    "gpuDrivenApiSupported={} gpuLogicalWriterAvailable={} fullGpuNativePackAvailable={} "
		    "fullGpuNativePackSubmitted={}",
		    evidenceLabel,
		    RhiPartitionedTlasOperationWriterPathToString(diagnostics.PtlasGpuUpdates.RequestedWriterPath),
		    RhiPartitionedTlasOperationWriterPathToString(diagnostics.PtlasGpuUpdates.SelectedWriterPath),
		    diagnostics.PtlasGpuUpdates.WriterSelectionReason,
		    diagnostics.PtlasGpuUpdates.LogicalUpdateCount,
		    diagnostics.PtlasGpuUpdates.NativeOperationCount,
		    diagnostics.PtlasGpuUpdates.ValidationMismatchCount,
		    diagnostics.PtlasGpuUpdates.GpuDrivenOperationApiSupported,
		    diagnostics.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable,
		    diagnostics.PtlasGpuUpdates.FullGpuNativePackAvailable,
		    diagnostics.PtlasGpuUpdates.FullGpuNativePackSubmitted);
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
