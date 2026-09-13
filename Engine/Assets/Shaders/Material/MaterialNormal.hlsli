#pragma once

float3 UnpackMaterialNormal(float3 encodedNormal, float normalScale)
{
	const float3 decoded = encodedNormal * 2.0f - 1.0f;
	return normalize(float3(decoded.xy * normalScale, decoded.z));
}
