#pragma once

#include "Debug/RenderViewModeConstants.hlsli"

namespace InstanceView
{
	uint HashInstanceId(uint instanceId)
	{
		uint hash = instanceId;
		hash ^= hash >> 16u;
		hash *= 0x7feb352du;
		hash ^= hash >> 15u;
		hash *= 0x846ca68bu;
		hash ^= hash >> 16u;
		return hash;
	}

	float3 MakeInstanceColor(uint instanceId)
	{
		const uint hash = HashInstanceId(instanceId);
		const float3 color = float3(
		    hash & 0xffu,
		    (hash >> 8u) & 0xffu,
		    (hash >> 16u) & 0xffu) / 255.0f;
		return 0.25f.xxx + color * 0.75f;
	}

	float3 ApplyInstanceDebugVisualization(float3 baseColor, uint instanceId)
	{
		switch (ViewModeIndex)
		{
			case ViewMode::InstanceGroups:
				return saturate(MakeInstanceColor(instanceId));
			default:
				return baseColor;
		}
	}
}  // namespace InstanceView
