#pragma once

#include "/Engine/Common/Math.hlsli"

float3 DecodeBc5TangentNormal(float2 encodedNormalXY, float normalScale)
{
	// Normal textures are cooked as BC5. Reconstruct the positive tangent-space
	// hemisphere from the two stored channels instead of interpreting the absent
	// blue channel as tangent-space -Z.
	const float2 decodedNormalXY = encodedNormalXY * 2.0f - 1.0f;
	const float normalZ = sqrt(saturate(1.0f - dot(decodedNormalXY, decodedNormalXY)));
	return SafeNormalize(float3(decodedNormalXY * normalScale, normalZ));
}
