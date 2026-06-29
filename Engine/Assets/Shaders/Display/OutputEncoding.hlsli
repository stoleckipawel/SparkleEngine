#ifndef SPARKLE_DISPLAY_OUTPUT_ENCODING_HLSLI
#define SPARKLE_DISPLAY_OUTPUT_ENCODING_HLSLI

#include "Common/Color.hlsli"

namespace OutputEncoding
{
	static const uint Linear = 0u;
	static const uint Srgb = 1u;

	float3 Encode(float3 displayLinearColor, uint outputEncoding)
	{
		return outputEncoding == Srgb ? CommonColor::EncodeSrgb(displayLinearColor) : saturate(displayLinearColor);
	}
}

#endif
