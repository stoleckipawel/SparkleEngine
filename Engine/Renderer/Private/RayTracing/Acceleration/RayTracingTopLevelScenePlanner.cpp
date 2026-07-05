#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"

#include "Debug/RendererCVars.h"
#include "RayTracing/Acceleration/RayTracingPtlasLogicalUpdateStream.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "SceneData/RenderSceneData.h"

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

void RayTracingTopLevelScenePlanner::PlanFrame(
    const RenderSceneData& sceneData,
    const DirectX::XMFLOAT3& cameraPosition,
    bool buildPartitionedTlasUpdateStream) noexcept
{
	if (m_impl == nullptr)
	{
		return;
	}

	m_impl->CurrentPartitionPlan = m_impl->PartitionPlanner.Build(
	    sceneData,
	    RayTracingPtlasPartitionPlannerConfig{
	        .PartitionsPerAxis = CVarRayTracingPartitionsPerAxis.Get(),
	        .PartitionUpdateMode = CVarRayTracingPtlasPartitionUpdateMode.Get(),
	        .MarkAllDynamicInPartition = CVarRayTracingPtlasMarkAllDynamicInPartition.Get(),
	        .CameraPosition = cameraPosition,
	        .ModeChangeDistance = CVarRayTracingPtlasModeChangeDistance.Get()});
	m_impl->CurrentLogicalUpdateStream =
	    buildPartitionedTlasUpdateStream ? m_impl->LogicalUpdateStream.Build(sceneData, m_impl->CurrentPartitionPlan)
	                                     : RayTracingPtlasLogicalUpdateStreamResult{};
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

void RayTracingTopLevelScenePlanner::Clear() noexcept
{
	if (m_impl != nullptr)
	{
		m_impl->PartitionPlanner.Clear();
		m_impl->CurrentPartitionPlan = {};
		m_impl->CurrentLogicalUpdateStream = {};
	}
}
