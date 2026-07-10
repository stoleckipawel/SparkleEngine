#ifndef SPARKLE_GEOMETRY_SCREEN_SPACE_HLSLI
#define SPARKLE_GEOMETRY_SCREEN_SPACE_HLSLI

#include "Resources/ConstantBuffers.hlsli"

float2 PixelCenterToNdc(uint2 pixelCoord)
{
	const float2 uv = (float2(pixelCoord) + 0.5f) * ViewportSizeInv;
	return float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
}

float2 PixelCenterToUnjitteredNdc(uint2 pixelCoord)
{
	return PixelCenterToNdc(pixelCoord) - JitterCurrent;
}

#endif
