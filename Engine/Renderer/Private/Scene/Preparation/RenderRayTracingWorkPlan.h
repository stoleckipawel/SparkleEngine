#pragma once

#include <cstdint>
#include <vector>

struct RenderRayTracingBlasInput final
{
	std::uint32_t PrimitiveIndex = 0u;
	std::uint32_t GpuSceneSlot = 0u;
};

struct RenderRayTracingWorkPlan final
{
	std::vector<RenderRayTracingBlasInput> BlasInputs;
	std::vector<std::uint32_t> ClassicTlasBlasInputIndices;
	std::vector<std::uint32_t> PartitionedTlasBlasInputIndices;
};
