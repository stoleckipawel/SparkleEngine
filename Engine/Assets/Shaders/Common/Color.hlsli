#ifndef SPARKLE_COMMON_COLOR_HLSLI
#define SPARKLE_COMMON_COLOR_HLSLI

namespace CommonColor
{
	static const float3 Rec709LuminanceWeights = float3(0.2126f, 0.7152f, 0.0722f);

	float3 ClampPositive(float3 color)
	{
		return max(color, 0.0f.xxx);
	}

	float LuminanceRec709(float3 linearRgb)
	{
		return dot(linearRgb, Rec709LuminanceWeights);
	}

	float3 EncodeSrgb(float3 linearColor)
	{
		const float3 color = saturate(linearColor);
		const float3 low = color * 12.92f;
		const float3 high = 1.055f * pow(color, 1.0f.xxx / 2.4f.xxx) - 0.055f.xxx;
		return lerp(low, high, step(0.0031308f.xxx, color));
	}
}

#endif
