#ifndef SPARKLE_RAY_RECONSTRUCTION_GUIDES_HLSLI
#define SPARKLE_RAY_RECONSTRUCTION_GUIDES_HLSLI

#include "/Engine/Lighting/SurfaceLighting.hlsli"
#include "/Engine/Passes/GBuffer/GBufferUtils.hlsli"
#include "/Engine/RayTracing/PathLighting.hlsli"

RWTexture2D<float4> RayReconstructionDiffuseAlbedo;
RWTexture2D<float4> RayReconstructionSpecularAlbedo;
RWTexture2D<float> RayReconstructionRoughness;
RWTexture2D<float> RayReconstructionSpecularHitDistance;

namespace RayReconstructionGuides
{
	float3 ComputeDiffuseReflectance(GBufferData gBuffer)
	{
		return saturate(gBuffer.BaseColor) * (1.0f - saturate(gBuffer.Metallic));
	}

	float3 EnvBRDFApprox2(float3 specularColor, float alpha, float NoV)
	{
		NoV = abs(NoV);

		const float4 X = float4(1.0f, NoV, NoV * NoV, NoV * NoV * NoV);
		const float4 Y = float4(1.0f, alpha, alpha * alpha, alpha * alpha * alpha);
		const float2x2 M1 = float2x2(0.99044f, -1.28514f, 1.29678f, -0.755907f);
		const float3x3 M2 = float3x3(1.0f, 2.92338f, 59.4188f, 20.3225f, -27.0302f, 222.592f, 121.563f, 626.13f, 316.627f);
		const float2x2 M3 = float2x2(0.0365463f, 3.32707f, 9.0632f, -9.04756f);
		const float3x3 M4 = float3x3(1.0f, 3.59685f, -1.36772f, 9.04401f, -16.3174f, 9.22949f, 5.56589f, 19.7886f, -20.2123f);

		float bias = dot(mul(M1, X.xy), Y.xy) * rcp(dot(mul(M2, X.xyw), Y.xyw));
		const float scale = dot(mul(M3, X.xy), Y.xy) * rcp(dot(mul(M4, X.xzw), Y.xyw));
		bias *= saturate(specularColor.g * 50.0f);
		return mad(specularColor, max(0.0f, scale), max(0.0f, bias));
	}

	float3 ComputeSpecularAlbedo(GBufferData gBuffer, float3 viewDirWorld)
	{
		const float3 normalWorld = normalize(gBuffer.NormalWorld);
		const float NoV = dot(normalWorld, normalize(viewDirWorld));
		const float alpha = gBuffer.Roughness * gBuffer.Roughness;
		const float3 f0 = SurfaceLighting::BuildF0(gBuffer.BaseColor, gBuffer.Metallic, gBuffer.DielectricF0);
		return EnvBRDFApprox2(f0, alpha, NoV);
	}

	void Clear(uint2 pixelCoord)
	{
		RayReconstructionDiffuseAlbedo[pixelCoord] = 0.0f.xxxx;
		RayReconstructionSpecularAlbedo[pixelCoord] = 0.0f.xxxx;
		RayReconstructionRoughness[pixelCoord] = 0.0f;
		RayReconstructionSpecularHitDistance[pixelCoord] = 0.0f;
	}

	void ClearSpecularHitDistance(uint2 pixelCoord)
	{
		RayReconstructionSpecularHitDistance[pixelCoord] = 0.0f;
	}

	void WriteSurface(uint2 pixelCoord, GBufferData gBuffer, float3 viewDirWorld)
	{
		RayReconstructionDiffuseAlbedo[pixelCoord] = float4(ComputeDiffuseReflectance(gBuffer), 1.0f);
		RayReconstructionSpecularAlbedo[pixelCoord] = float4(ComputeSpecularAlbedo(gBuffer, viewDirWorld), 1.0f);
		RayReconstructionRoughness[pixelCoord] = saturate(gBuffer.Roughness);
	}

	void WriteSpecularHitDistance(uint2 pixelCoord, RayTracingPathLighting::Result path, float3 primaryPositionWorld, bool specularSelected)
	{
		const bool validSpecularHit = specularSelected && path.FirstLighting.Hit;
		const float hitDistance = validSpecularHit ? length(path.FirstLighting.HitPositionWorld - primaryPositionWorld) : 0.0f;
		RayReconstructionSpecularHitDistance[pixelCoord] = hitDistance;
	}
}

#endif
