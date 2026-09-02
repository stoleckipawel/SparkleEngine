#pragma once

#include "/Engine/Resources/ViewUniformData.hlsli"

#include "/Engine/Common/Hash.hlsli"
#include "/Engine/Debug/RenderViewModeConstants.hlsli"

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
}
