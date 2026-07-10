#ifndef SPARKLE_INDIRECT_LIGHTING_OUTPUTS_HLSLI
#define SPARKLE_INDIRECT_LIGHTING_OUTPUTS_HLSLI

#include "Lighting/SurfaceLighting.hlsli"
#include "RayTracing/Shadows/RayTracedShadowVisibility.hlsli"
#include "RayTracing/PathLighting.hlsli"

RWTexture2D<float4> IndirectDiffuse;
RWTexture2D<float4> IndirectSpecular;
RWTexture2D<float4> IndirectDiffuseAlbedo;
RWTexture2D<float4> IndirectSpecularAlbedo;
RWTexture2D<float4> IndirectMaterialGuide;
RWTexture2D<float4> IndirectSpecularSampleGuide;

namespace IndirectLightingOutputs
{
	void Clear(uint2 pixelCoord, bool producerValid)
	{
		IndirectDiffuse[pixelCoord] = float4(0.0f.xxx, producerValid ? 1.0f : 0.0f);
		IndirectSpecular[pixelCoord] = 0.0f.xxxx;
		IndirectDiffuseAlbedo[pixelCoord] = 0.0f.xxxx;
		IndirectSpecularAlbedo[pixelCoord] = 0.0f.xxxx;
		IndirectMaterialGuide[pixelCoord] = 0.0f.xxxx;
		IndirectSpecularSampleGuide[pixelCoord] = 0.0f.xxxx;
	}

	void ClearRadianceAndSpecularGuide(uint2 pixelCoord)
	{
		IndirectDiffuse[pixelCoord] = 0.0f.xxxx;
		IndirectSpecular[pixelCoord] = 0.0f.xxxx;
		IndirectSpecularSampleGuide[pixelCoord] = 0.0f.xxxx;
	}

	void WriteSurfaceGuides(uint2 pixelCoord, GBufferData gBuffer)
	{
		const float3 diffuseAlbedo = gBuffer.BaseColor * (1.0f - gBuffer.Metallic);
		const float3 specularAlbedo = SurfaceLighting::BuildF0(gBuffer.BaseColor, gBuffer.Metallic, gBuffer.DielectricF0);
		IndirectDiffuseAlbedo[pixelCoord] = float4(diffuseAlbedo, 1.0f);
		IndirectSpecularAlbedo[pixelCoord] = float4(specularAlbedo, 1.0f);
		IndirectMaterialGuide[pixelCoord] = float4(gBuffer.Roughness, gBuffer.Metallic, gBuffer.DielectricF0, 1.0f);
	}

	void WriteSpecularSampleGuide(uint2 pixelCoord, RayTracingPathLighting::Result path, bool specularSelected)
	{
		IndirectSpecularSampleGuide[pixelCoord] = float4(
		    specularSelected ? max(path.FirstLighting.HitDistance, 0.0f) : 0.0f,
		    specularSelected ? 1.0f : 0.0f,
		    float(RayTracingPathSample::LobeSpecular),
		    specularSelected && path.FirstLighting.Hit ? 1.0f : 0.0f);
	}
}

#endif
