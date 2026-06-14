#pragma once

#include <cstdint>
#include <vector>

struct RayTracingMeshInstanceDebugData final
{
	std::vector<std::uint32_t> PackedDebugVisualizationDataByRenderInstance;

	std::uint32_t GetPackedDebugVisualizationData(std::uint32_t renderInstanceIndex) const noexcept;
};

struct RayTracingSceneFramePlan final
{
	RayTracingMeshInstanceDebugData MeshInstanceDebugData;
};
