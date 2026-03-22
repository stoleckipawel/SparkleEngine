#pragma once

namespace Lighting
{
	namespace Shadow
	{
		Texture2D<float> ShadowMap0 : register(t5);
		Texture2D<float> ShadowMap1 : register(t6);

		float SampleShadowDepth(float2 uv, uint mapIndex)
		{
			if (mapIndex == 0)
				return ShadowMap0.SampleLevel(SamplerPointNoMipClamp, uv, 0).r;
			return ShadowMap1.SampleLevel(SamplerPointNoMipClamp, uv, 0).r;
		}
	}
}