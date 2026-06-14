#include "PCH.h"

#include "RayTracing/RayTracingSceneDiagnostics.h"

#include "RHI/Public/Core/RhiBackendSelection.h"

static const auto g_rayTracingSceneLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

void RayTracingSceneDiagnostics::LogSceneUpdate(
    const RayTracingCapabilityReport& capabilityReport,
    const RayTracingBlasCache::BuildStats& blasStats,
    const RayTracingClassicTlasBuilder::BuildStats& tlasStats) noexcept
{
	if (!capabilityReport.Core.SupportsRayTracing)
	{
		return;
	}

	if (m_hasLoggedSceneSummary && m_lastReferencedMeshCount == blasStats.referencedMeshCount &&
	    m_lastBuiltBlasCount == blasStats.builtBlasCount && m_lastReusedBlasCount == blasStats.reusedBlasCount &&
	    m_lastCandidateInstanceCount == tlasStats.Candidates.InstanceCount &&
	    m_lastTlasInstanceCount == tlasStats.Build.InstanceCount &&
	    m_lastMissingGpuMeshCount == tlasStats.Candidates.MissingGpuMeshCount &&
	    m_lastRejectedBlasCount == tlasStats.Candidates.RejectedBlasCount &&
	    m_lastPartitionCount == tlasStats.PtlasPlanner.PartitionCount &&
	    m_lastDirtyTransformCount == tlasStats.PtlasPlanner.DirtyTransformCount &&
	    m_lastMovedPartitionCount == tlasStats.PtlasPlanner.MovedPartitionCount &&
	    m_lastGlobalPartitionInstanceCount == tlasStats.PtlasPlanner.GlobalPartitionInstanceCount &&
	    m_lastDuplicateStableIndexCount == tlasStats.PtlasPlanner.DuplicateStableIndexCount &&
	    m_lastPartitionOverflow == tlasStats.PtlasPlanner.Overflow &&
	    m_lastBuiltTlas == tlasStats.Build.Built)
	{
		return;
	}

	m_hasLoggedSceneSummary = true;
	m_lastReferencedMeshCount = blasStats.referencedMeshCount;
	m_lastBuiltBlasCount = blasStats.builtBlasCount;
	m_lastReusedBlasCount = blasStats.reusedBlasCount;
	m_lastCandidateInstanceCount = tlasStats.Candidates.InstanceCount;
	m_lastTlasInstanceCount = tlasStats.Build.InstanceCount;
	m_lastMissingGpuMeshCount = tlasStats.Candidates.MissingGpuMeshCount;
	m_lastRejectedBlasCount = tlasStats.Candidates.RejectedBlasCount;
	m_lastPartitionCount = tlasStats.PtlasPlanner.PartitionCount;
	m_lastDirtyTransformCount = tlasStats.PtlasPlanner.DirtyTransformCount;
	m_lastMovedPartitionCount = tlasStats.PtlasPlanner.MovedPartitionCount;
	m_lastGlobalPartitionInstanceCount = tlasStats.PtlasPlanner.GlobalPartitionInstanceCount;
	m_lastDuplicateStableIndexCount = tlasStats.PtlasPlanner.DuplicateStableIndexCount;
	m_lastPartitionOverflow = tlasStats.PtlasPlanner.Overflow;
	m_lastBuiltTlas = tlasStats.Build.Built;

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
	    tlasStats.Candidates.InstanceCount,
	    tlasStats.Build.InstanceCount,
	    tlasStats.Candidates.MissingGpuMeshCount,
	    tlasStats.Candidates.RejectedBlasCount,
	    tlasStats.Build.Built,
	    RhiRayTracingTopLevelProviderToString(capabilityReport.TopLevelProvider.SelectedProvider),
	    capabilityReport.TopLevelProvider.SelectionReason,
	    RhiPartitionedTlasProviderToString(capabilityReport.PartitionedTlas.Provider),
	    capabilityReport.PartitionedTlas.Supported,
	    tlasStats.PtlasPlanner.PartitionCount,
	    tlasStats.PtlasPlanner.DirtyTransformCount,
	    tlasStats.PtlasPlanner.MovedPartitionCount,
	    tlasStats.PtlasPlanner.GlobalPartitionInstanceCount,
	    tlasStats.PtlasPlanner.DuplicateStableIndexCount,
	    tlasStats.PtlasPlanner.Overflow,
	    capabilityReport.PartitionedTlas.CapabilityStatusReason);
}
