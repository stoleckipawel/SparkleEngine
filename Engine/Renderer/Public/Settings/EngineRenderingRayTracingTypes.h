#pragma once

enum class EnginePtlasPartitionTopology
{
	XZ2D,
	XYZ3D,
};

enum class EnginePtlasPartitionUpdateMode
{
	AlwaysUpdatePartition,
	AlwaysMoveDynamicToGlobal,
	UpdatePartitionNearbyMoveToGlobalOtherwise,
};
