#ifndef SPARKLE_REFERENCE_PATH_TRACER_UNIFORM_HLSLI
#define SPARKLE_REFERENCE_PATH_TRACER_UNIFORM_HLSLI

cbuffer ReferencePathTracerConstants
{
	uint ReferencePathTracerSamplesPerPixel;
	uint ReferencePathTracerBounceCount;
	float ReferencePathTracerNormalBias;
	float ReferencePathTracerMaxDistance;
};

#endif
