#pragma once

#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"

namespace DirectX
{
	struct XMFLOAT3;
}

struct PreparedRenderScene;

class RenderRayTracingViewPlanner final
{
public:
	RayTracingPtlasPartitionPlan Build(const PreparedRenderScene& preparedScene, const DirectX::XMFLOAT3& cameraPosition) noexcept;
	void Reset() noexcept;

private:
	RayTracingPtlasPartitionPlanner m_partitionPlanner;
};
