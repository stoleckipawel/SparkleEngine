#include "PCH.h"

#include "RayTracing/RayTracingTopLevelScenePlanner.h"

#include "Debug/RendererCVars.h"
#include "RayTracing/RayTracingPtlasLogicalUpdateStream.h"
#include "RayTracing/RayTracingPtlasOperationWriterPolicy.h"
#include "RayTracing/RayTracingPtlasPartitionPlanner.h"

struct RayTracingTopLevelScenePlanner::Impl final
{
	RayTracingPtlasPartitionPlanner PartitionPlanner;
	RayTracingPtlasLogicalUpdateStream LogicalUpdateStream;
	RayTracingPtlasPartitionPlan CurrentPartitionPlan;
	RayTracingPtlasLogicalUpdateStreamResult CurrentLogicalUpdateStream;
};

RayTracingTopLevelScenePlanner::RayTracingTopLevelScenePlanner() noexcept :
    m_impl(std::make_unique<Impl>())
{
}

RayTracingTopLevelScenePlanner::~RayTracingTopLevelScenePlanner() noexcept = default;

RayTracingSceneFramePlan RayTracingTopLevelScenePlanner::PlanFrame(const RenderSceneData& sceneData) noexcept
{
	if (m_impl == nullptr)
	{
		return {};
	}

	m_impl->CurrentPartitionPlan = m_impl->PartitionPlanner.Build(
	    sceneData,
	    RayTracingPtlasPartitionPlannerConfig{
	        .PartitionsPerAxis = CVarRayTracingPartitionsPerAxis.Get(),
	        .EnableGlobalPartition = CVarRayTracingGlobalPartition.Get()});
	m_impl->CurrentLogicalUpdateStream = m_impl->LogicalUpdateStream.Build(sceneData, m_impl->CurrentPartitionPlan);

	RayTracingSceneFramePlan framePlan{};
	framePlan.MeshInstanceDebugData.PackedDebugVisualizationDataByRenderInstance.reserve(
	    m_impl->CurrentPartitionPlan.Indices.Entries.size());
	for (const RayTracingPtlasPartitionEntry& entry : m_impl->CurrentPartitionPlan.Indices.Entries)
	{
		framePlan.MeshInstanceDebugData.PackedDebugVisualizationDataByRenderInstance.push_back(entry.DebugVisualization.PackedData);
	}
	return framePlan;
}

const RayTracingPtlasPartitionPlan* RayTracingTopLevelScenePlanner::GetCurrentPartitionPlan() const noexcept
{
	return m_impl != nullptr ? &m_impl->CurrentPartitionPlan : nullptr;
}

const RayTracingPtlasLogicalUpdateStreamResult* RayTracingTopLevelScenePlanner::GetCurrentLogicalUpdateStream() const noexcept
{
	return m_impl != nullptr ? &m_impl->CurrentLogicalUpdateStream : nullptr;
}

RayTracingClassicTlasBuilder::BuildStats RayTracingTopLevelScenePlanner::BuildClassicTlas(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingClassicTlasBuilder& classicTlasBuilder,
    RayTracingBlasCache& blasCache,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	return classicTlasBuilder.Build(
	    cmd,
	    sceneData,
	    m_impl != nullptr ? &m_impl->CurrentPartitionPlan : nullptr,
	    blasCache,
	    diagnostics);
}

RayTracingTopLevelScenePlannerMetrics RayTracingTopLevelScenePlanner::GetCurrentPlannerMetrics() const noexcept
{
	if (m_impl == nullptr)
	{
		return {};
	}

	const ERhiPartitionedTlasOperationWriterPath requestedWriterPath =
	    RayTracingPtlasOperationWriterPolicyResolver::ResolveRequestedPath();
	return RayTracingTopLevelScenePlannerMetrics{
	    .PartitionCount = m_impl->CurrentPartitionPlan.Counts.PartitionCount,
	    .DirtyTransformCount = m_impl->CurrentPartitionPlan.Counts.DirtyTransformCount,
	    .MovedPartitionCount = m_impl->CurrentPartitionPlan.Counts.MovedPartitionCount,
	    .GlobalPartitionInstanceCount = m_impl->CurrentPartitionPlan.Counts.GlobalPartitionInstanceCount,
	    .DuplicateStableIndexCount = m_impl->CurrentPartitionPlan.Counts.DuplicateStableIndexCount,
	    .Overflow = m_impl->CurrentPartitionPlan.Validation.HasPartitionOverflow,
	    .GpuUpdates =
	        RayTracingPtlasGpuUpdateMetrics{
	            .RequestedWriterPath = requestedWriterPath,
	            .SelectedWriterPath = ERhiPartitionedTlasOperationWriterPath::CpuPack,
	            .WriterSelectionReason = RayTracingPtlasOperationWriterPolicyResolver::ResolveUnsupportedReason(requestedWriterPath),
	            .LogicalUpdateCount = m_impl->CurrentLogicalUpdateStream.LogicalUpdateCount,
	            .NativeOperationCount = 0,
	            .ValidationMismatchCount = m_impl->CurrentLogicalUpdateStream.SkippedInvalidInstanceCount,
	            .FullGpuNativePackSupported = false,
	            .FullGpuNativePackSubmitted = false}};
}

void RayTracingTopLevelScenePlanner::Clear() noexcept
{
	if (m_impl != nullptr)
	{
		m_impl->PartitionPlanner.Clear();
		m_impl->CurrentPartitionPlan = {};
		m_impl->CurrentLogicalUpdateStream = {};
	}
}
