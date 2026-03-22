#pragma once

namespace Lighting
{
	namespace Shadow
	{
		Texture2D<float> ShadowMap0 : register(t5);

		float SampleShadowDepth(float2 uv)
		{
			return ShadowMap0.SampleLevel(SamplerPointNoMipClamp, uv, 0).r;
		}
	}
}