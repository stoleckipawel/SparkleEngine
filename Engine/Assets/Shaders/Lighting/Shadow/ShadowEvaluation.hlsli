#pragma once

#include "Lighting/Shadow/ShadowMaps.hlsli"

namespace Lighting
{
	namespace Shadow
	{
		float ComputeShadowFactor(float3 positionWorld, float3 normalWorld, float3 lightDirection)
		{
			const float4 lightClipPosition = mul(float4(positionWorld, 1.0f), ViewLighting.Shadow.ViewProjMTX);
			const float3 lightNdc = lightClipPosition.xyz / lightClipPosition.w;
			if (lightNdc.x < -1.0f || lightNdc.x > 1.0f || lightNdc.y < -1.0f || lightNdc.y > 1.0f || lightNdc.z < 0.0f || lightNdc.z > 1.0f)
			{
				return 1.0f;
			}

			const float2 uv = float2(lightNdc.x * 0.5f + 0.5f, 0.5f - lightNdc.y * 0.5f);
			const float normalBias = (1.0f - saturate(dot(normalWorld, lightDirection))) * ViewLighting.Shadow.NormalBias;
			const float biasedReceiverDepth = lightNdc.z + ViewLighting.Shadow.DepthBias + normalBias;
			const float shadowDepth = SampleShadowDepth(uv);

			return biasedReceiverDepth >= shadowDepth ? 1.0f : 0.0f;
		}
	}
}