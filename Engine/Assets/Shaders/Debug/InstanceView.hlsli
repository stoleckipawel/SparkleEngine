#pragma once

#include "Resources/ViewUniformData.hlsli"

#include "Common/Hash.hlsli"
#include "Debug/RenderViewModeConstants.hlsli"

namespace InstanceView
{
	float3 ApplyInstanceVisualization(float3 baseColor, uint gpuSceneSlot)
	{
		if (ViewModeIndex == ViewMode::GpuSceneInstances)
		{
			return HashIdColor(gpuSceneSlot, 0u);
		}
		return baseColor;
	}
} // namespace InstanceView
