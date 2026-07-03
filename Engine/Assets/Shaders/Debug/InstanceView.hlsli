#pragma once

#include "Common/Hash.hlsli"
#include "Debug/RenderViewModeConstants.hlsli"

namespace InstanceView
{
	float3 ApplyDebugVisualization(float3 baseColor, uint debugData)
	{
		if (ViewModeIndex == ViewMode::InstanceGroups)
		{
			return HashIdColor(debugData, 0u);
		}
		return baseColor;
	}
}  // namespace InstanceView
