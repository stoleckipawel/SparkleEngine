#pragma once

#include "/Engine/Resources/ViewUniformData.hlsli"

#include "/Engine/Common/Hash.hlsli"
#include "/Engine/Resources/RenderViewModeConstants.hlsli"

namespace InstanceView
{
	float3 ApplyInstanceVisualization(float3 baseColor, uint gpuSceneSlot)
	{
		if (RenderViewModeIndex == RenderViewMode::GpuSceneInstances)
		{
			return HashIdColor(gpuSceneSlot, 0u);
		}
		return baseColor;
	}
}
