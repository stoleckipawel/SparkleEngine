#pragma once

float3 HashIdColor(uint instanceId, uint primitiveIndex)
{
	const uint seed = instanceId * 1664525u + primitiveIndex * 1013904223u + 0x9E3779B9u;
	const uint r = (seed >> 0u) & 255u;
	const uint g = (seed >> 8u) & 255u;
	const uint b = (seed >> 16u) & 255u;
	return 0.15f.xxx + 0.85f * (float3(r, g, b) / 255.0f);
}

float3 SafeNormalize(float3 value, float3 fallback)
{
	const float lengthSquared = dot(value, value);
	return lengthSquared > 1.0e-8f ? value * rsqrt(lengthSquared) : fallback;
}

float3 SafeTransformNormal(float3 localNormal, float3x3 worldInvTransposeMatrix, float3 fallback)
{
	return SafeNormalize(mul(localNormal, worldInvTransposeMatrix), fallback);
}

float3 SafeTransformDirection(float3 localDirection, float3x3 worldMatrix, float3 fallback)
{
	return SafeNormalize(mul(localDirection, worldMatrix), fallback);
}

void BuildOrthonormalBasis(float3 normalWorld, out float3 tangentWorld, out float3 bitangentWorld)
{
	const float sign = normalWorld.z >= 0.0f ? 1.0f : -1.0f;
	const float a = -1.0f / (sign + normalWorld.z);
	const float b = normalWorld.x * normalWorld.y * a;
	tangentWorld = SafeNormalize(float3(1.0f + sign * normalWorld.x * normalWorld.x * a, sign * b, -sign * normalWorld.x), float3(1.0f, 0.0f, 0.0f));
	bitangentWorld = SafeNormalize(float3(b, sign + normalWorld.y * normalWorld.y * a, -normalWorld.y), float3(0.0f, 1.0f, 0.0f));
}

float3 OrthonormalizeTangent(float3 tangentWorld, float3 normalWorld)
{
	const float3 projectedTangent = tangentWorld - normalWorld * dot(tangentWorld, normalWorld);
	float3 fallbackTangent = 0.0f.xxx;
	float3 fallbackBitangent = 0.0f.xxx;
	BuildOrthonormalBasis(normalWorld, fallbackTangent, fallbackBitangent);
	return SafeNormalize(projectedTangent, fallbackTangent);
}

float3 ComputeHitBitangent(float3 normalWorld, float3 tangentWorld, float tangentSign)
{
	float3 fallbackTangent = 0.0f.xxx;
	float3 fallbackBitangent = 0.0f.xxx;
	BuildOrthonormalBasis(normalWorld, fallbackTangent, fallbackBitangent);
	return SafeNormalize(tangentSign * cross(normalWorld, tangentWorld), fallbackBitangent);
}

float3 UnpackMaterialNormal(float2 encodedNormal)
{
	const float2 normalXY = encodedNormal * 2.0f - 1.0f;
	const float normalZ = sqrt(saturate(1.0f - dot(normalXY, normalXY)));
	return normalize(float3(normalXY, normalZ));
}

float3 TransformHitNormalToWorld(
    float3 normalTangent,
    float3 normalWorld,
    float3 tangentWorld,
    float3 bitangentWorld)
{
	const float3x3 TBN = float3x3(tangentWorld, bitangentWorld, normalWorld);
	return SafeNormalize(mul(normalTangent, TBN), normalWorld);
}
