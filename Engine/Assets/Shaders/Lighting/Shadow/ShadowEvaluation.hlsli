#pragma once

#include "Lighting/Shadow/ShadowSampling.hlsli"

namespace Shadow
{
	float ComputeCascadeShadowFactor(
		float3 positionWorld,
		float3 normalWorld,
		float3 lightDirection,
		uint lightIndex,
		uint cascadeIndex)
	{
		const ShadowConstantBufferData shadowData = GetDirectionalShadowCascade(lightIndex, cascadeIndex);
		const float4 lightClipPosition = mul(float4(positionWorld, 1.0f), shadowData.ViewProjMTX);
		const float3 lightNdc = lightClipPosition.xyz / lightClipPosition.w;
		if (lightNdc.x < -1.0f || lightNdc.x > 1.0f || lightNdc.y < -1.0f || lightNdc.y > 1.0f || lightNdc.z < 0.0f || lightNdc.z > 1.0f)
		{
			return 1.0f;
		}

		const float2 uv = float2(lightNdc.x * 0.5f + 0.5f, 0.5f - lightNdc.y * 0.5f);
		const float normalBias = (1.0f - saturate(dot(normalize(normalWorld), lightDirection))) * shadowData.NormalBias;
		const float biasedReceiverDepth = saturate(lightNdc.z + shadowData.DepthBias + normalBias);

		return SampleDirectionalShadowVisibility(
			uv,
			biasedReceiverDepth,
			shadowData.ShadowMapSize,
			lightIndex,
			cascadeIndex);
	}

	float ComputeShadowFactor(float3 positionWorld, float3 normalWorld, float3 lightDirection, uint lightIndex)
	{
		const float cameraDistance = length(positionWorld - Camera.Position);
		const ShadowConstantBufferData nearCascade = GetDirectionalShadowCascade(lightIndex, 0);

		const float nearFactor = ComputeCascadeShadowFactor(
			positionWorld,
			normalWorld,
			lightDirection,
			lightIndex,
			0);
		const float splitDepth = nearCascade.CascadeFarDepth;
		const float blendWeight = GetCascadeBlendWeight(cameraDistance, splitDepth);

		if (blendWeight <= 0.0f)
		{
			return nearFactor;
		}

		const float farFactor = ComputeCascadeShadowFactor(
			positionWorld,
			normalWorld,
			lightDirection,
			lightIndex,
			1);
		return lerp(nearFactor, farFactor, blendWeight);
	}
}