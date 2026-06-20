#pragma once

#include "Debug/InstanceView.hlsli"
#include "Debug/RenderViewModeConstants.hlsli"

namespace RayTracingPtlasDebugVisualization
{
	static const uint PartitionDebugPartitionMask = 0x000FFFFFu;
	static const uint PartitionDebugDynamicInstance = 1u << 27u;
	static const uint PartitionDebugDirtyTransform = 1u << 28u;
	static const uint PartitionDebugMovedPartition = 1u << 29u;
	static const uint PartitionDebugGlobalPartition = 1u << 30u;
	static const uint PartitionDebugInvalid = 1u << 31u;

	float3 MakePartitionColor(uint rayTracingPtlasDebugVisualizationData)
	{
		const uint partitionId = rayTracingPtlasDebugVisualizationData & PartitionDebugPartitionMask;
		return InstanceView::MakeInstanceColor(partitionId);
	}

	float3 ApplyPartitionUpdateVisualization(float3 baseColor, uint rayTracingPtlasDebugVisualizationData)
	{
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugInvalid) != 0u)
		{
			return float3(1.0f, 0.0f, 1.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugGlobalPartition) != 0u)
		{
			return float3(0.9f, 0.1f, 1.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugDynamicInstance) != 0u)
		{
			return float3(0.0f, 0.95f, 1.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugMovedPartition) != 0u)
		{
			return float3(1.0f, 0.45f, 0.05f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugDirtyTransform) != 0u)
		{
			return float3(1.0f, 0.9f, 0.1f);
		}
		return lerp(baseColor, float3(0.08f, 0.12f, 0.18f), 0.28f);
	}

	float3 ApplyInstanceMovementVisualization(float3 baseColor, uint rayTracingPtlasDebugVisualizationData)
	{
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugInvalid) != 0u)
		{
			return float3(1.0f, 0.0f, 1.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugMovedPartition) != 0u)
		{
			return float3(1.0f, 0.1f, 0.05f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugDynamicInstance) != 0u)
		{
			return lerp(baseColor, float3(0.1f, 1.0f, 0.75f), 0.75f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugDirtyTransform) != 0u)
		{
			return float3(0.0f, 0.85f, 1.0f);
		}
		return lerp(baseColor, float3(0.08f, 0.11f, 0.13f), 0.35f);
	}

	float3 ApplyTopLevelModeVisualization(float3 baseColor, uint rayTracingPtlasDebugVisualizationData)
	{
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugInvalid) != 0u)
		{
			return float3(1.0f, 0.0f, 1.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugGlobalPartition) != 0u)
		{
			return float3(0.7f, 0.2f, 1.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugDynamicInstance) != 0u)
		{
			return float3(0.0f, 0.9f, 1.0f);
		}
		return lerp(baseColor, float3(0.1f, 0.45f, 1.0f), 0.32f);
	}

	float3 ApplyNativeOperationVisualization(float3 baseColor, uint rayTracingPtlasDebugVisualizationData)
	{
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugMovedPartition) != 0u)
		{
			return float3(1.0f, 0.2f, 0.05f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugDirtyTransform) != 0u)
		{
			return float3(1.0f, 0.85f, 0.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugDynamicInstance) != 0u)
		{
			return float3(0.1f, 0.8f, 1.0f);
		}
		return lerp(baseColor, float3(0.10f, 0.09f, 0.06f), 0.35f);
	}

	float3 ApplyProviderStatusVisualization(float3 baseColor, uint rayTracingPtlasDebugVisualizationData)
	{
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugInvalid) != 0u)
		{
			return float3(1.0f, 0.0f, 1.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugGlobalPartition) != 0u)
		{
			return float3(0.85f, 0.0f, 1.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugDynamicInstance) != 0u)
		{
			return float3(0.0f, 0.9f, 1.0f);
		}
		return lerp(baseColor, MakePartitionColor(rayTracingPtlasDebugVisualizationData), 0.35f);
	}

	float3 ApplyDebugVisualization(float3 baseColor, uint rayTracingPtlasDebugVisualizationData)
	{
		switch (ViewModeIndex)
		{
			case ViewMode::RayTracingPartitions:
				return saturate(lerp(baseColor, MakePartitionColor(rayTracingPtlasDebugVisualizationData), 0.34f));
			case ViewMode::RayTracingPartitionUpdates:
				return saturate(ApplyPartitionUpdateVisualization(baseColor, rayTracingPtlasDebugVisualizationData));
			case ViewMode::RayTracingInstanceMovement:
				return saturate(ApplyInstanceMovementVisualization(baseColor, rayTracingPtlasDebugVisualizationData));
			case ViewMode::RayTracingGpuDrivenUpdates:
				return saturate(ApplyPartitionUpdateVisualization(baseColor, rayTracingPtlasDebugVisualizationData));
			case ViewMode::RayTracingTopLevelMode:
				return saturate(ApplyTopLevelModeVisualization(baseColor, rayTracingPtlasDebugVisualizationData));
			case ViewMode::RayTracingNativeOperations:
				return saturate(ApplyNativeOperationVisualization(baseColor, rayTracingPtlasDebugVisualizationData));
			case ViewMode::RayTracingProviderStatus:
				return saturate(ApplyProviderStatusVisualization(baseColor, rayTracingPtlasDebugVisualizationData));
			default:
				return baseColor;
		}
	}
}  // namespace RayTracingPtlasDebugVisualization
