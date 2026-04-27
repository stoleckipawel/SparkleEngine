#pragma once

#include "Lighting/Shadow/ShadowCascade.hlsli"

namespace Shadow
{
	Texture2D<float> ShadowMap0;
	Texture2D<float> ShadowMap1;
	Texture2D<float> ShadowMap2;
	Texture2D<float> ShadowMap3;

	float4 GatherShadowDepth(float2 uv, uint mapIndex, int2 offset)
	{
		if (mapIndex == 0)
			return ShadowMap0.GatherRed(SamplerLinearNoMipClamp, uv, offset);
		if (mapIndex == 1)
			return ShadowMap1.GatherRed(SamplerLinearNoMipClamp, uv, offset);
		if (mapIndex == 2)
			return ShadowMap2.GatherRed(SamplerLinearNoMipClamp, uv, offset);
		return ShadowMap3.GatherRed(SamplerLinearNoMipClamp, uv, offset);
	}

	float4 GatherDirectionalShadowDepth(float2 uv, uint lightIndex, uint cascadeIndex, int2 offset)
	{
		return GatherShadowDepth(uv, GetShadowMapIndex(lightIndex, cascadeIndex), offset);
	}
}  // namespace Shadow