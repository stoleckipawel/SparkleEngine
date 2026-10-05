#ifndef SPARKLE_DIRECT_LIGHTING_UNIFORM_HLSLI
#define SPARKLE_DIRECT_LIGHTING_UNIFORM_HLSLI

cbuffer DirectLightingConstants
{
	uint DirectLightingEvaluateDiffuse;
	uint DirectLightingEvaluateSpecular;
	uint DirectLightingEvaluateSubsurface;
	uint DirectLightingEvaluateShadows;
};

#endif
