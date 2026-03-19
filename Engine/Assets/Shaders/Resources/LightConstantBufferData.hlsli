#pragma once

#include "Resources/RendererSettings.hlsli"

struct DirectionalLightConstantBufferData
{
	float3 Direction;
	float Intensity;

	float3 Color;
	float Padding;
};

struct PointLightConstantBufferData
{
	float3 Position;
	float Intensity;

	float3 Color;
	float Radius;

	uint Enabled;
	float3 Padding;
};

struct PerViewLightingConstantBufferData
{
	uint DirectionalLightCount;
	uint PointLightCount;
	uint PaddingCounts[2];

	DirectionalLightConstantBufferData DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
	PointLightConstantBufferData PointLights[MAX_POINT_LIGHTS];
	float4 Padding[13];
};