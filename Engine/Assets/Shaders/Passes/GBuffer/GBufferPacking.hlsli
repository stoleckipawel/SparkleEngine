#ifndef SPARKLE_GBUFFER_PACKING_HLSLI
#define SPARKLE_GBUFFER_PACKING_HLSLI

namespace GBufferPacking
{
	float PackOutputAlpha(float alpha, uint alphaMode, uint blendedAlphaMode)
	{
		return alphaMode == blendedAlphaMode ? alpha : 1.0f;
	}

	float4 PackBaseColor(float3 baseColor, float alpha, uint alphaMode, uint blendedAlphaMode)
	{
		return float4(baseColor, PackOutputAlpha(alpha, alphaMode, blendedAlphaMode));
	}

	float4 PackNormal(float3 normalWorld)
	{
		return float4(normalize(normalWorld), 0.0f);
	}

	float4 PackMaterial(float metallic, float roughness, float ambientOcclusion, float dielectricF0)
	{
		return float4(saturate(metallic), saturate(roughness), saturate(ambientOcclusion), saturate(dielectricF0));
	}

	float4 PackEmissive(float3 emissive)
	{
		return float4(emissive, 0.0f);
	}

	float4 PackSubsurface(float3 subsurfaceColor, float subsurfaceStrength)
	{
		return float4(saturate(subsurfaceColor), saturate(subsurfaceStrength));
	}

	float PackSkyDeviceZ()
	{
		return 0.0f;
	}

	float4 PackSkyBaseColor()
	{
		return float4(0.0f.xxx, 1.0f);
	}

	float4 PackSkyNormal()
	{
		return float4(0.0f, 0.0f, 1.0f, 0.0f);
	}

	float4 PackSkyMaterial()
	{
		return PackMaterial(0.0f, 1.0f, 1.0f, 0.04f);
	}

	float4 PackSkyEmissive()
	{
		return 0.0f.xxxx;
	}

	float4 PackSkySubsurface()
	{
		return 0.0f.xxxx;
	}
}

#endif
