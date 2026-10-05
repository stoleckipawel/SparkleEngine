#ifndef SPARKLE_RESTIR_INDIRECT_LIGHTING_UNIFORM_HLSLI
#define SPARKLE_RESTIR_INDIRECT_LIGHTING_UNIFORM_HLSLI

cbuffer RestirIndirectConstants
{
	uint RestirIndirectBounceCount;
	uint RestirIndirectTemporalReuse;
	uint RestirIndirectSpatialReuse;
	uint RestirIndirectEvaluateDiffuse;
	uint RestirIndirectEvaluateSpecular;
	uint RestirIndirectTraceSecondaryShadows;
	uint RestirIndirectWriteReconstructionGuides;
	uint RestirIndirectPadding;
};

#endif
