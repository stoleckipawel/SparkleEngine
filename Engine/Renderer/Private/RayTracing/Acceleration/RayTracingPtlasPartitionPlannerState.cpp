#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"

#include "Renderer/Public/SceneData/MeshDraw.h"

#include <algorithm>
#include <cmath>

static const auto g_rayTracingPtlasPartitionPlannerStateLogger =
    Logging::GetOrCreateLogger("Renderer.RayTracing.PtlasPartitionPlannerState");

class RayTracingPtlasPartitionPlannerStateConstants final
{
public:
	static constexpr std::uint32_t kMaxPlannerPartitionsPerAxis = 64;
};

void RayTracingPtlasPartitionPlanner::Clear() noexcept
{
	m_previousInstances.clear();
	m_partitionStates.clear();
}

void RayTracingPtlasPartitionPlanner::ValidateConfig(const RayTracingPtlasPartitionPlannerConfig& config) noexcept
{
	if (config.PartitionsPerAxis == 0u
	    || config.PartitionsPerAxis > RayTracingPtlasPartitionPlannerStateConstants::kMaxPlannerPartitionsPerAxis
	    || config.ModeChangeDistance < 0.0f || config.TransformDirtyEpsilon < 0.0f)
	{
		Diagnostics::Fatal(
		    g_rayTracingPtlasPartitionPlannerStateLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS planner configuration is outside its structural bounds.");
	}
}

bool RayTracingPtlasPartitionPlanner::IsTransformDirty(
    const DirectX::XMFLOAT4X4& current,
    const DirectX::XMFLOAT4X4& previous,
    float epsilon) noexcept
{
	const float* currentValues = &current._11;
	const float* previousValues = &previous._11;
	for (std::size_t index = 0; index < 16; ++index)
	{
		if (std::abs(currentValues[index] - previousValues[index]) > epsilon)
		{
			return true;
		}
	}
	return false;
}

bool RayTracingPtlasPartitionPlanner::IsGlobalPartitionEligible(const MeshDraw& draw) noexcept
{
	return draw.Geometry.MeshKind != RenderMeshKind::Static;
}
