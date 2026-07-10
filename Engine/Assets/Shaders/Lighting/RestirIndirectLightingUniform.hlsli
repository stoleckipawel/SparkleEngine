#ifndef SPARKLE_RESTIR_INDIRECT_LIGHTING_UNIFORM_HLSLI
#define SPARKLE_RESTIR_INDIRECT_LIGHTING_UNIFORM_HLSLI

cbuffer RestirIndirectLightingUniformData
{
	uint RestirIndirectBounceCount;
	float RestirIndirectNormalBias;
	float RestirIndirectMaxDistance;
	uint RestirIndirectPadding;
};

#endif
