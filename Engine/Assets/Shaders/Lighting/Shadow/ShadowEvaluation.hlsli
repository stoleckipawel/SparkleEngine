#pragma once

#include "Lighting/Shadow/ShadowMaps.hlsli"

namespace Lighting
{
	namespace Shadow
	{
		float ComputeShadowFactor(float3 positionWorld, float3 normalWorld, float3 lightDirection, uint lightIndex)
		{
			const ShadowConstantBufferData shadowData = ViewLighting.Shadows[lightIndex];
			const float4 lightClipPosition = mul(float4(positionWorld, 1.0f), shadowData.ViewProjMTX);
			const float3 lightNdc = lightClipPosition.xyz / lightClipPosition.w;
			if (lightNdc.x < -1.0f || lightNdc.x > 1.0f || lightNdc.y < -1.0f || lightNdc.y > 1.0f || lightNdc.z < 0.0f || lightNdc.z > 1.0f)
			{
				return 1.0f;
			}

			const float2 uv = float2(lightNdc.x * 0.5f + 0.5f, 0.5f - lightNdc.y * 0.5f);
			const float normalBias = (1.0f - saturate(dot(normalize(normalWorld), lightDirection))) * shadowData.NormalBias;
			const float biasedReceiverDepth = saturate(lightNdc.z + shadowData.DepthBias + normalBias);

			return SampleShadowVisibility(uv, biasedReceiverDepth, shadowData.ShadowMapSize, lightIndex);
		}
	}
}