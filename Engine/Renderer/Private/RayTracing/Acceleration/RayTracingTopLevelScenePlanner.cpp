#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"

#include "Debug/RendererCVars.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "Scene/Preparation/PreparedRenderScene.h"

struct RayTracingTopLevelScenePlanner::Impl final
{
	RayTracingPtlasPartitionPlanner PartitionPlanner;
	RayTracingPtlasPartitionPlan CurrentPartitionPlan;
};

RayTracingTopLevelScenePlanner::RayTracingTopLevelScenePlanner() noexcept :
    m_impl(std::make_unique<Impl>())
{
}

RayTracingTopLevelScenePlanner::~RayTracingTopLevelScenePlanner() noexcept = default;

void RayTracingTopLevelScenePlanner::PlanFrame(const PreparedRenderScene& preparedScene, const DirectX::XMFLOAT3& cameraPosition) noexcept
{
	if (m_impl == nullptr)
	{
		return;
	}

	m_impl->CurrentPartitionPlan = m_impl->PartitionPlanner.Build(
	    preparedScene,
	    RayTracingPtlasPartitionPlannerConfig{
	        .PartitionsPerAxis = CVarRayTracingPartitionsPerAxis.Get(),
	        .PartitionUpdateMode = CVarRayTracingPtlasPartitionUpdateMode.Get(),
	        .MarkAllDynamicInPartition = CVarRayTracingPtlasMarkAllDynamicInPartition.Get(),
	        .CameraPosition = cameraPosition,
	        .ModeChangeDistance = CVarRayTracingPtlasModeChangeDistance.Get()});
}

const RayTracingPtlasPartitionPlan* RayTracingTopLevelScenePlanner::GetCurrentPartitionPlan() const noexcept
{
	return m_impl != nullptr ? &m_impl->CurrentPartitionPlan : nullptr;
}

RayTracingClassicTlasBuilder::BuildStats RayTracingTopLevelScenePlanner::BuildClassicTlas(
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    RayTracingClassicTlasBuilder& classicTlasBuilder,
    RayTracingBlasCache& blasCache,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	return classicTlasBuilder.Build(commandContext, preparedScene, blasCache, diagnostics);
}

void RayTracingTopLevelScenePlanner::Clear() noexcept
{
	if (m_impl != nullptr)
	{
		m_impl->PartitionPlanner.Clear();
		m_impl->CurrentPartitionPlan = {};
	}
}
