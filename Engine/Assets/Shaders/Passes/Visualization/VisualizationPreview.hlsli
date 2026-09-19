#pragma once

float3 VisualizeScalar(float value)
{
	return saturate(value).xxx;
}

float3 VisualizeNormal(float3 normalWorld)
{
	const float lengthSquared = dot(normalWorld, normalWorld);
	const float3 normalized = lengthSquared > 0.0f ? normalWorld * rsqrt(lengthSquared) : float3(0.0f, 0.0f, 1.0f);
	return normalized * 0.5f + 0.5f;
}
