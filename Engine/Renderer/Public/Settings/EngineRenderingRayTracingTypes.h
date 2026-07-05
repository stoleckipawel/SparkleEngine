#pragma once

enum class EnginePtlasPartitionUpdateMode
{
	AlwaysUpdatePartition,
	AlwaysMoveDynamicToGlobal,
	UpdatePartitionNearbyMoveToGlobalOtherwise,
};
