#pragma once

#include "Common/Sampling.hlsli"

float3 OrthonormalizeTangent(float3 tangent, float3 normal)
{
	const float3 projectedTangent = tangent - normal * dot(tangent, normal);
	float3 fallbackTangent = 0.0f.xxx;
	float3 fallbackBitangent = 0.0f.xxx;
	CommonSampling::BuildOrthonormalBasis(normal, fallbackTangent, fallbackBitangent);
	return SafeNormalize(projectedTangent, fallbackTangent);
}

float3 ComputeBitangentFromSign(float3 normal, float3 tangent, float tangentSign)
{
	float3 fallbackTangent = 0.0f.xxx;
	float3 fallbackBitangent = 0.0f.xxx;
	CommonSampling::BuildOrthonormalBasis(normal, fallbackTangent, fallbackBitangent);
	return SafeNormalize(tangentSign * cross(normal, tangent), fallbackBitangent);
}

float3 TransformTangentNormalToWorld(float3 normalTangent, float3 normalWorld, float3 tangentWorld, float3 bitangentWorld)
{
	const float3x3 TBN = float3x3(tangentWorld, bitangentWorld, normalWorld);
	return SafeNormalize(mul(normalTangent, TBN), normalWorld);
}
