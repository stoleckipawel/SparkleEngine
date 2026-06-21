#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"

#include "Debug/RendererCVars.h"
#include "RayTracing/Acceleration/RayTracingPtlasLogicalUpdateStream.h"
#include "RayTracing/Acceleration/RayTracingPtlasOperationWriterPolicy.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "SceneData/RenderSceneData.h"

struct RayTracingTopLevelScenePlanner::Impl final
{
	RayTracingPtlasPartitionPlanner PartitionPlanner;
	RayTracingPtlasLogicalUpdateStream LogicalUpdateStream;
	RayTracingPtlasPartitionPlan CurrentPartitionPlan;
	RayTracingPtlasLogicalUpdateStreamResult CurrentLogicalUpdateStream;
	std::uint32_t LastRenderInstanceCount = 0;
};

RayTracingTopLevelScenePlanner::RayTracingTopLevelScenePlanner() noexcept :
    m_impl(std::make_unique<Impl>())
{
}

RayTracingTopLevelScenePlanner::~RayTracingTopLevelScenePlanner() noexcept = default;

RayTracingSceneFramePlan RayTracingTopLevelScenePlanner::PlanFrame(
    const RenderSceneData& sceneData,
    const DirectX::XMFLOAT3& cameraPosition,
    bool buildPartitionedTlasUpdateStream) noexcept
{
	if (m_impl == nullptr)
	{
		return {};
	}

	m_impl->CurrentPartitionPlan = m_impl->PartitionPlanner.Build(
	    sceneData,
	    RayTracingPtlasPartitionPlannerConfig{
	        .PartitionsPerAxis = CVarRayTracingPartitionsPerAxis.Get(),
	        .PartitionTopology = CVarRayTracingPtlasPartitionTopology.Get(),
	        .PartitionUpdateMode = CVarRayTracingPtlasPartitionUpdateMode.Get(),
	        .EnableGlobalPartition = true,
	        .MarkAllDynamicInPartition = CVarRayTracingPtlasMarkAllDynamicInPartition.Get(),
	        .CameraPosition = cameraPosition,
	        .ModeChangeDistance = CVarRayTracingPtlasModeChangeDistance.Get()});
	m_impl->CurrentLogicalUpdateStream =
	    buildPartitionedTlasUpdateStream ? m_impl->LogicalUpdateStream.Build(sceneData, m_impl->CurrentPartitionPlan)
	                                     : RayTracingPtlasLogicalUpdateStreamResult{};
	m_impl->LastRenderInstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());

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
	    .TotalRenderInstanceCount = m_impl->LastRenderInstanceCount,
	    .TraceableInstanceCount = m_impl->CurrentPartitionPlan.Counts.CandidateInstanceCount,
	    .StaticTraceableInstanceCount = m_impl->CurrentPartitionPlan.Counts.StaticInstanceCount,
	    .DynamicTraceableInstanceCount = m_impl->CurrentPartitionPlan.Counts.DynamicInstanceCount,
	    .PartitionsPerAxis = m_impl->CurrentPartitionPlan.Counts.PartitionsPerAxis,
	    .PartitionCount = m_impl->CurrentPartitionPlan.Counts.PartitionCount,
	    .GridPartitionCount = m_impl->CurrentPartitionPlan.Counts.GridPartitionCount,
	    .DirtyTransformCount = m_impl->CurrentPartitionPlan.Counts.DirtyTransformCount,
	    .MovedPartitionCount = m_impl->CurrentPartitionPlan.Counts.MovedPartitionCount,
	    .GlobalPartitionEligibleCount = m_impl->CurrentPartitionPlan.Counts.GlobalPartitionEligibleCount,
	    .GlobalPartitionInstanceCount = m_impl->CurrentPartitionPlan.Counts.GlobalPartitionInstanceCount,
	    .ActivePartitionCount = m_impl->CurrentPartitionPlan.Counts.ActivePartitionCount,
	    .MaxPartitionActivityCount = m_impl->CurrentPartitionPlan.Counts.MaxPartitionActivityCount,
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
	            .GpuDrivenOperationApiSupported = false,
	            .GpuLogicalUpdateWriterAvailable = false,
	            .FullGpuNativePackAvailable = false,
	            .FullGpuNativePackSubmitted = false}};
}

void RayTracingTopLevelScenePlanner::Clear() noexcept
{
	if (m_impl != nullptr)
	{
		m_impl->PartitionPlanner.Clear();
		m_impl->CurrentPartitionPlan = {};
		m_impl->CurrentLogicalUpdateStream = {};
		m_impl->LastRenderInstanceCount = 0;
	}
}
