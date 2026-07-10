#pragma once

#include <cstdint>

enum class GBufferMode : std::uint8_t
{
	Rasterized,
	Raytraced
};

enum class LightingMode : std::uint8_t
{
	RestirPathTraced,
	ReferencePathTraced
};

enum class RayTracingPtlasPartitionUpdateMode : std::uint8_t
{
	AlwaysUpdatePartition,
	AlwaysMoveDynamicToGlobal,
	UpdatePartitionNearbyMoveToGlobalOtherwise,
};
