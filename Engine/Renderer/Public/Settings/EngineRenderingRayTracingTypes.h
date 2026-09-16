#pragma once

#include <cstdint>

enum class GBufferAlgorithm : std::uint8_t
{
	Rasterized,
	RayTracing
};

enum class RayTracingPtlasPartitionUpdateMode : std::uint8_t
{
	AlwaysUpdatePartition,
	AlwaysMoveDynamicToGlobal,
	UpdatePartitionNearbyMoveToGlobalOtherwise,
};
