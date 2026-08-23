#ifndef SPARKLE_LIGHTING_SKY_UNIFORM_HLSLI
#define SPARKLE_LIGHTING_SKY_UNIFORM_HLSLI

cbuffer Sky
{
	float3 SkyColor;
	float SkyBrightness;
	uint SkyEnabled;
	uint3 SkyPadding;
};

#endif
