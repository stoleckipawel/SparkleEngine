#ifndef SPARKLE_PATH_TRACED_LIGHTING_UNIFORM_HLSLI
#define SPARKLE_PATH_TRACED_LIGHTING_UNIFORM_HLSLI

cbuffer PathTracedLightingConstants
{
	uint PathTracedLightingSamplesPerPixel;
	uint PathTracedLightingBounceCount;
	float PathTracedLightingNormalBias;
	float PathTracedLightingMaxDistance;
};

#endif
