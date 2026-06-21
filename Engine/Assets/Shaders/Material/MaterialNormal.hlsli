#pragma once

float3 UnpackMaterialNormal(float2 encodedNormal)
{
	const float2 normalXY = encodedNormal * 2.0f - 1.0f;
	const float normalZ = sqrt(saturate(1.0f - dot(normalXY, normalXY)));
	return normalize(float3(normalXY, normalZ));
}
