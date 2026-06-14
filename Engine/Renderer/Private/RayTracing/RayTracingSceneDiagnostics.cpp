#include "PCH.h"

#include "RayTracing/RayTracingSceneDiagnostics.h"

#include "RHI/Public/Core/RhiBackendSelection.h"

static const auto g_rayTracingSceneLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

void RayTracingSceneDiagnostics::LogSceneUpdate(
    const RayTracingCapabilityReport& capabilityReport,
    ERhiRayTracingTopLevelProvider activeProvider,
    const char* activeProviderReason,
    const RayTracingBlasCache::BuildStats& blasStats,
    const RayTracingTopLevelAccelerationStructureBuildStats& topLevelStats) noexcept
{
	if (!capabilityReport.Core.SupportsRayTracing)
	{
		return;
	}

	if (m_hasLoggedSceneSummary && m_lastReferencedMeshCount == blasStats.referencedMeshCount &&
	    m_lastBuiltBlasCount == blasStats.builtBlasCount && m_lastReusedBlasCount == blasStats.reusedBlasCount &&
	    m_lastCandidateInstanceCount == topLevelStats.Candidates.InstanceCount &&
	    m_lastTlasInstanceCount == topLevelStats.Build.InstanceCount &&
	    m_lastMissingGpuMeshCount == topLevelStats.Candidates.MissingGpuMeshCount &&
	    m_lastRejectedBlasCount == topLevelStats.Candidates.RejectedBlasCount &&
	    m_lastPartitionCount == topLevelStats.PtlasPlanner.PartitionCount &&
	    m_lastDirtyTransformCount == topLevelStats.PtlasPlanner.DirtyTransformCount &&
	    m_lastMovedPartitionCount == topLevelStats.PtlasPlanner.MovedPartitionCount &&
	    m_lastGlobalPartitionInstanceCount == topLevelStats.PtlasPlanner.GlobalPartitionInstanceCount &&
	    m_lastDuplicateStableIndexCount == topLevelStats.PtlasPlanner.DuplicateStableIndexCount &&
	    m_lastPartitionOverflow == topLevelStats.PtlasPlanner.Overflow &&
	    m_lastBuiltTlas == topLevelStats.Build.Built)
	{
		return;
	}

	m_hasLoggedSceneSummary = true;
	m_lastReferencedMeshCount = blasStats.referencedMeshCount;
	m_lastBuiltBlasCount = blasStats.builtBlasCount;
	m_lastReusedBlasCount = blasStats.reusedBlasCount;
	m_lastCandidateInstanceCount = topLevelStats.Candidates.InstanceCount;
	m_lastTlasInstanceCount = topLevelStats.Build.InstanceCount;
	m_lastMissingGpuMeshCount = topLevelStats.Candidates.MissingGpuMeshCount;
	m_lastRejectedBlasCount = topLevelStats.Candidates.RejectedBlasCount;
	m_lastPartitionCount = topLevelStats.PtlasPlanner.PartitionCount;
	m_lastDirtyTransformCount = topLevelStats.PtlasPlanner.DirtyTransformCount;
	m_lastMovedPartitionCount = topLevelStats.PtlasPlanner.MovedPartitionCount;
	m_lastGlobalPartitionInstanceCount = topLevelStats.PtlasPlanner.GlobalPartitionInstanceCount;
	m_lastDuplicateStableIndexCount = topLevelStats.PtlasPlanner.DuplicateStableIndexCount;
	m_lastPartitionOverflow = topLevelStats.PtlasPlanner.Overflow;
	m_lastBuiltTlas = topLevelStats.Build.Built;

	SPDLOG_LOGGER_INFO(
	    g_rayTracingSceneLogger,
	    "RenderRayTracingScene: backend={} supportsRT={} inlineRayQuery={} referencedMeshes={} builtBlas={} reusedBlas={} "
	    "candidateInstances={} tlasInstances={} missingGpuMeshData={} rejectedBlas={} builtTlas={} topLevelProvider={}({}) "
	    "partitionedTlasProvider={} supported={} plannedPartitions={} dirtyTransforms={} movedPartitions={} globalPartitionInstances={} "
	    "duplicateStableIndices={} partitionOverflow={} reason={}.",
	    RhiBackendApiToString(capabilityReport.BackendApi),
	    capabilityReport.Core.SupportsRayTracing,
	    capabilityReport.Core.SupportsInlineRayQuery,
	    blasStats.referencedMeshCount,
	    blasStats.builtBlasCount,
	    blasStats.reusedBlasCount,
	    topLevelStats.Candidates.InstanceCount,
	    topLevelStats.Build.InstanceCount,
	    topLevelStats.Candidates.MissingGpuMeshCount,
	    topLevelStats.Candidates.RejectedBlasCount,
	    topLevelStats.Build.Built,
	    RhiRayTracingTopLevelProviderToString(activeProvider),
	    activeProviderReason != nullptr ? activeProviderReason : "not-reported",
	    RhiPartitionedTlasProviderToString(capabilityReport.PartitionedTlas.Provider),
	    capabilityReport.PartitionedTlas.Supported,
	    topLevelStats.PtlasPlanner.PartitionCount,
	    topLevelStats.PtlasPlanner.DirtyTransformCount,
	    topLevelStats.PtlasPlanner.MovedPartitionCount,
	    topLevelStats.PtlasPlanner.GlobalPartitionInstanceCount,
	    topLevelStats.PtlasPlanner.DuplicateStableIndexCount,
	    topLevelStats.PtlasPlanner.Overflow,
	    capabilityReport.PartitionedTlas.CapabilityStatusReason);
}
