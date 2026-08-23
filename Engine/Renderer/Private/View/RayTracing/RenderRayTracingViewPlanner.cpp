#include "PCH.h"

#include "View/RayTracing/RenderRayTracingViewPlanner.h"

#include "Debug/RendererCVars.h"
#include "Scene/Preparation/PreparedRenderScene.h"

RayTracingPtlasPartitionPlan RenderRayTracingViewPlanner::Build(
    const PreparedRenderScene& preparedScene,
    const DirectX::XMFLOAT3& cameraPosition) noexcept
{
	return m_partitionPlanner.Build(
	    preparedScene,
	    RayTracingPtlasPartitionPlannerConfig{
	        .PartitionsPerAxis = CVarRayTracingPartitionsPerAxis.Get(),
	        .PartitionUpdateMode = CVarRayTracingPtlasPartitionUpdateMode.Get(),
	        .MarkAllDynamicInPartition = CVarRayTracingPtlasMarkAllDynamicInPartition.Get(),
	        .CameraPosition = cameraPosition,
	        .ModeChangeDistance = CVarRayTracingPtlasModeChangeDistance.Get()});
}

void RenderRayTracingViewPlanner::Reset() noexcept
{
	m_partitionPlanner.Clear();
}
