#pragma once

#include "Resources/RenderConfig.hlsli"
#include "Resources/ShadowConstantBufferData.hlsli"

struct DirectionalLightConstantBufferData
{
	float3 Direction;
	float Intensity;

	float3 Color;
	float _pad0;
};

struct PerViewLightingConstantBufferData
{
	uint DirectionalLightCount;
	uint3 PaddingCounts;

	DirectionalLightConstantBufferData DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
	ShadowConstantBufferData Shadows[MAX_DIRECTIONAL_LIGHTS];
};
