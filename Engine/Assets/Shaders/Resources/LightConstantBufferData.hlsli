#pragma once

struct DirectionalLightConstantBufferData
{
	float3 Direction;
	// Illuminance in lux.
	float Illuminance;

	float3 Color;
	float AngularSizeRadians;

	uint CastShadow;
	uint3 Padding;
};

struct PointLightConstantBufferData
{
	float3 Position;
	float Range;

	float3 Color;
	// Luminous intensity in candela.
	float LuminousIntensity;

	float3 DistanceAttenuationCoefficients;
	float Radius;

	uint CastShadow;
	uint3 Padding;
};

struct SpotLightConstantBufferData
{
	float3 Position;
	float Range;

	float3 Direction;
	float InnerAngleCosine;

	float3 Color;
	// Luminous intensity in candela.
	float LuminousIntensity;

	float3 DistanceAttenuationCoefficients;
	float Radius;

	float OuterAngleCosine;
	uint CastShadow;
	uint2 Padding;
};

struct RectLightConstantBufferData
{
	float3 Position;
	float Width;

	float3 Direction;
	float Height;

	float3 Tangent;
	// Luminance in candela per square meter.
	float Luminance;

	float3 Color;
	uint CastShadow;
};

struct ViewLightingData
{
	uint DirectionalLightCount;
	uint PointLightCount;
	uint SpotLightCount;
	uint RectLightCount;
};

ConstantBuffer<ViewLightingData> ViewLighting;
StructuredBuffer<DirectionalLightConstantBufferData> DirectionalLights;
StructuredBuffer<PointLightConstantBufferData> PointLights;
StructuredBuffer<SpotLightConstantBufferData> SpotLights;
StructuredBuffer<RectLightConstantBufferData> RectLights;
