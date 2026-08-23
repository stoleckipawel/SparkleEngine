#pragma once

#include "/Engine/Common/Constants.hlsli"

float Pow5(float x)
{
	const float x2 = x * x;
	return x2 * x2 * x;
}

float3 SafeNormalize(float3 v)
{
	const float len = length(v);
	return len > EPSILON ? v / len : float3(0.0f, 0.0f, 1.0f);
}

float3 SafeNormalize(float3 v, float3 fallback)
{
	const float lengthSquared = dot(v, v);
	return lengthSquared > 1.0e-8f ? v * rsqrt(lengthSquared) : fallback;
}

float Remap(float value, float inMin, float inMax, float outMin, float outMax)
{
	return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}
