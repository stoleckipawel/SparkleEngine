#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"

#include "Renderer/Public/SceneData/MeshDraw.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr std::uint32_t kMaxPlannerPartitionsPerAxis = 64;
}

void RayTracingPtlasPartitionPlanner::Clear() noexcept
{
	m_previousInstances.clear();
	m_partitionStates.clear();
	m_frameIndex = 0;
}

RayTracingPtlasPartitionPlannerConfig RayTracingPtlasPartitionPlanner::SanitizeConfig(RayTracingPtlasPartitionPlannerConfig config) noexcept
{
	config.PartitionsPerAxis = std::clamp(config.PartitionsPerAxis, 1u, kMaxPlannerPartitionsPerAxis);
	config.ModeChangeDistance = (std::max)(config.ModeChangeDistance, 0.0f);
	config.TransformDirtyEpsilon = (std::max)(config.TransformDirtyEpsilon, 0.0f);
	return config;
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
