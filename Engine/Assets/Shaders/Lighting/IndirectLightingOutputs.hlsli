#ifndef SPARKLE_INDIRECT_LIGHTING_OUTPUTS_HLSLI
#define SPARKLE_INDIRECT_LIGHTING_OUTPUTS_HLSLI

RWTexture2D<float4> IndirectDiffuse;
RWTexture2D<float4> IndirectSpecular;

namespace IndirectLightingOutputs
{
	void Clear(uint2 pixelCoord, bool producerValid)
	{
		IndirectDiffuse[pixelCoord] = float4(0.0f.xxx, producerValid ? 1.0f : 0.0f);
		IndirectSpecular[pixelCoord] = 0.0f.xxxx;
	}

	void ClearRadiance(uint2 pixelCoord)
	{
		IndirectDiffuse[pixelCoord] = 0.0f.xxxx;
		IndirectSpecular[pixelCoord] = 0.0f.xxxx;
	}
}

#endif
