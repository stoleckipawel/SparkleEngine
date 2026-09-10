#pragma once

#include <cstdint>

enum class GBufferAlgorithm : std::uint8_t
{
	Rasterized,
	RayTracing
};

enum class RayTracingExecutionMode : std::uint8_t
{
	Automatic,
	Inline,
	Pipeline,
};

enum class RayTracingPtlasPartitionUpdateMode : std::uint8_t
{
	AlwaysUpdatePartition,
	AlwaysMoveDynamicToGlobal,
	UpdatePartitionNearbyMoveToGlobalOtherwise,
};
