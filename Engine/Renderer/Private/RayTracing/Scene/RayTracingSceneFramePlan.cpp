#include "PCH.h"

#include "RayTracing/Scene/RayTracingSceneFramePlan.h"

std::uint32_t RayTracingMeshInstanceDebugData::GetPackedDebugVisualizationData(std::uint32_t renderInstanceIndex) const noexcept
{
	return renderInstanceIndex < PackedDebugVisualizationDataByRenderInstance.size()
	           ? PackedDebugVisualizationDataByRenderInstance[renderInstanceIndex]
	           : 0u;
}
