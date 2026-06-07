#pragma once

#include "Resources/RenderConfig.hlsli"

struct DirectionalLightConstantBufferData
{
	float3 Direction;
	float Intensity;

	float3 Color;
	float AngularDiameter;

	uint CastShadow;
	uint3 Padding;
};

struct PointLightConstantBufferData
{
	float3 Position;
	float Range;

	float3 Color;
	float Intensity;

	float SourceRadius;
	uint CastShadow;
	uint2 Padding;
};

struct SpotLightConstantBufferData
{
	float3 Position;
	float Range;

	float3 Direction;
	float InnerConeCosine;

	float3 Color;
	float Intensity;

	float OuterConeCosine;
	uint CastShadow;
	float SourceRadius;
	uint Padding;
};

struct PerViewLightingConstantBufferData
{
	uint DirectionalLightCount;
	uint PointLightCount;
	uint SpotLightCount;
	uint PaddingCount;

	DirectionalLightConstantBufferData DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
	PointLightConstantBufferData PointLights[MAX_POINT_LIGHTS];
	SpotLightConstantBufferData SpotLights[MAX_SPOT_LIGHTS];
};
