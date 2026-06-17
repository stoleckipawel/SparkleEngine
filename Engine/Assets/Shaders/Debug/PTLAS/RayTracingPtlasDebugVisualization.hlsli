#pragma once

#include "Debug/InstanceView.hlsli"
#include "Debug/RenderViewModeConstants.hlsli"

namespace RayTracingPtlasDebugVisualization
{
	static const uint PartitionDebugPartitionMask = 0x000FFFFFu;
	static const uint PartitionDebugActivityShift = 20u;
	static const uint PartitionDebugActivityMask = 0x0FF00000u;
	static const uint PartitionDebugDirtyTransform = 1u << 28u;
	static const uint PartitionDebugMovedPartition = 1u << 29u;
	static const uint PartitionDebugGlobalPartition = 1u << 30u;
	static const uint PartitionDebugInvalid = 1u << 31u;

	float3 MakePartitionColor(uint rayTracingPtlasDebugVisualizationData)
	{
		const uint partitionId = rayTracingPtlasDebugVisualizationData & PartitionDebugPartitionMask;
		return InstanceView::MakeInstanceColor(partitionId);
	}

	float GetPartitionActivity(uint rayTracingPtlasDebugVisualizationData)
	{
		return (float)((rayTracingPtlasDebugVisualizationData & PartitionDebugActivityMask) >> PartitionDebugActivityShift) / 255.0f;
	}

	float3 Desaturate(float3 color, float saturation)
	{
		const float luminance = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
		return lerp(luminance.xxx, color, saturation);
	}

	float3 ApplyPartitionUpdateVisualization(float3 baseColor, uint rayTracingPtlasDebugVisualizationData)
	{
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugInvalid) != 0u)
		{
			return float3(1.0f, 0.0f, 1.0f);
		}
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugGlobalPartition) != 0u)
		{
			const float activity = max(GetPartitionActivity(rayTracingPtlasDebugVisualizationData), 0.55f);
			return lerp(float3(0.42f, 0.08f, 0.65f), float3(1.0f, 0.1f, 1.0f), activity);
		}

		const float3 partitionColor = MakePartitionColor(rayTracingPtlasDebugVisualizationData);
		float activity = GetPartitionActivity(rayTracingPtlasDebugVisualizationData);
		if ((rayTracingPtlasDebugVisualizationData & (PartitionDebugMovedPartition | PartitionDebugDirtyTransform)) != 0u)
		{
			activity = max(activity, 0.8f);
		}

		const float inactiveSaturation = 0.18f;
		const float activeSaturation = lerp(0.42f, 1.0f, activity);
		const float activeBrightness = lerp(0.34f, 1.18f, activity);
		const float3 inactiveColor = Desaturate(partitionColor, inactiveSaturation) * 0.42f;
		const float3 activeColor = Desaturate(partitionColor, activeSaturation) * activeBrightness;
		return lerp(inactiveColor, activeColor, saturate(activity));
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
		if ((rayTracingPtlasDebugVisualizationData & PartitionDebugDirtyTransform) != 0u)
		{
			return float3(0.0f, 0.85f, 1.0f);
		}
		return baseColor * 0.2f;
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
		return lerp(baseColor, float3(0.1f, 0.45f, 1.0f), 0.65f);
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
		return baseColor * 0.18f;
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
		return lerp(baseColor, MakePartitionColor(rayTracingPtlasDebugVisualizationData), 0.75f);
	}

	float3 ApplyDebugVisualization(float3 baseColor, uint rayTracingPtlasDebugVisualizationData)
	{
		switch (ViewModeIndex)
		{
			case ViewMode::RayTracingPartitions:
				return saturate(MakePartitionColor(rayTracingPtlasDebugVisualizationData));
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
