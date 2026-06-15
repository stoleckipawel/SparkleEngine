#pragma once

#include <cstdint>

struct RayTracingPtlasPlannerMetrics final
{
	std::uint32_t TotalRenderInstanceCount = 0;
	std::uint32_t TraceableInstanceCount = 0;
	std::uint32_t StaticTraceableInstanceCount = 0;
	std::uint32_t DynamicTraceableInstanceCount = 0;
	std::uint32_t PartitionsPerAxis = 0;
	std::uint32_t PartitionCount = 0;
	std::uint32_t GridPartitionCount = 0;
	std::uint32_t DirtyTransformCount = 0;
	std::uint32_t MovedPartitionCount = 0;
	std::uint32_t GlobalPartitionEligibleCount = 0;
	std::uint32_t GlobalPartitionInstanceCount = 0;
	std::uint32_t DuplicateStableIndexCount = 0;
	bool Overflow = false;
};
