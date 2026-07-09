#pragma once

#include <cstdint>

enum class GBufferMode : std::uint8_t
{
	Rasterized,
	Raytraced
};

enum class LightingMode : std::uint8_t
{
	Raytraced,
	PathTraced
};

enum class RayTracingPtlasPartitionUpdateMode : std::uint8_t
{
	AlwaysUpdatePartition,
	AlwaysMoveDynamicToGlobal,
	UpdatePartitionNearbyMoveToGlobalOtherwise,
};
