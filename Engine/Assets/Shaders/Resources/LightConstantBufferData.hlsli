#pragma once

struct DirectionalLightConstantBufferData
{
	float3 Direction;
	// Illuminance in lux.
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
	// Luminous intensity in candela.
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
	// Luminous intensity in candela.
	float Intensity;

	float OuterConeCosine;
	uint CastShadow;
	float SourceRadius;
	uint Padding;
};

struct ViewLightingData
{
	uint DirectionalLightCount;
	uint PointLightCount;
	uint SpotLightCount;
	uint PaddingCount;
};
