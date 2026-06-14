#pragma once

#include <cstdint>

struct RayTracingPtlasPlannerMetrics final
{
	std::uint32_t PartitionCount = 0;
	std::uint32_t DirtyTransformCount = 0;
	std::uint32_t MovedPartitionCount = 0;
	std::uint32_t GlobalPartitionInstanceCount = 0;
	std::uint32_t DuplicateStableIndexCount = 0;
	bool Overflow = false;
};
