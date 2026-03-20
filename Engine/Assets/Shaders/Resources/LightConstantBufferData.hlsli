#pragma once

#include "Resources/RenderConfig.hlsli"

struct DirectionalLightConstantBufferData
{
	float3 Direction;
	float Intensity;

	float3 Color;
	float Padding;
};

struct PerViewLightingConstantBufferData
{
	uint DirectionalLightCount;
	uint PaddingCounts[3];

	DirectionalLightConstantBufferData DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};