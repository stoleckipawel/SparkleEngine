#ifndef SPARKLE_GBUFFER_UTILS_HLSLI
#define SPARKLE_GBUFFER_UTILS_HLSLI

#include "Passes/Deferred/GBufferPacking.hlsli"

Texture2D GBufferBaseColor;
Texture2D GBufferNormal;
Texture2D GBufferMaterial;
Texture2D GBufferEmissive;
Texture2D GBufferSubsurface;
Texture2D GBufferDeviceZ;

struct GBufferData
{
	float3 BaseColor;
	float Alpha;
	float3 NormalWorld;
	float DeviceZ;
	float Metallic;
	float Roughness;
	float AmbientOcclusion;
	float DielectricF0;
	float3 Emissive;
	float3 SubsurfaceColor;
	float SubsurfaceStrength;
};

float GetSkyDeviceZValue()
{
	return GBufferPacking::PackSkyDeviceZ();
}

bool IsSkyPixel(float deviceZ)
{
	return deviceZ <= GetSkyDeviceZValue() + 1.0e-6f;
}

float3 DecodeGBufferNormal(float3 normalWorld)
{
	const float lengthSquared = dot(normalWorld, normalWorld);
	return lengthSquared > 0.0f ? normalWorld * rsqrt(lengthSquared) : float3(0.0f, 0.0f, 1.0f);
}

float DecodeGBufferDielectricF0(float storedDielectricF0)
{
	return saturate(storedDielectricF0);
}

float LoadGBufferDeviceZ(uint2 pixelCoord)
{
	return GBufferDeviceZ.Load(int3(pixelCoord, 0)).r;
}

GBufferData LoadGBuffer(uint2 pixelCoord)
{
	const int3 pixel = int3(pixelCoord, 0);
	const float4 baseColorSample = GBufferBaseColor.Load(pixel);
	const float4 normalSample = GBufferNormal.Load(pixel);
	const float4 materialSample = GBufferMaterial.Load(pixel);
	const float4 emissiveSample = GBufferEmissive.Load(pixel);
	const float4 subsurfaceSample = GBufferSubsurface.Load(pixel);

	GBufferData gBuffer;
	gBuffer.BaseColor = saturate(baseColorSample.rgb);
	gBuffer.Alpha = baseColorSample.a;
	gBuffer.NormalWorld = DecodeGBufferNormal(normalSample.xyz);
	gBuffer.DeviceZ = LoadGBufferDeviceZ(pixelCoord);
	gBuffer.Metallic = saturate(materialSample.r);
	gBuffer.Roughness = saturate(materialSample.g);
	gBuffer.AmbientOcclusion = saturate(materialSample.b);
	gBuffer.DielectricF0 = DecodeGBufferDielectricF0(materialSample.a);
	gBuffer.Emissive = max(emissiveSample.rgb, 0.0f);
	gBuffer.SubsurfaceColor = saturate(subsurfaceSample.rgb);
	gBuffer.SubsurfaceStrength = saturate(subsurfaceSample.a);
	return gBuffer;
}

float3 ReconstructGBufferWorldPosition(uint2 pixelCoord, float deviceZ, float4x4 invView, float4x4 invProjection)
{
	const float2 uv = (float2(pixelCoord) + 0.5f) * ViewportSizeInv;
	const float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
	const float4 positionClip = float4(ndc, deviceZ, 1.0f);
	const float4 positionView = mul(positionClip, invProjection);
	const float4 positionWorld = mul(float4(positionView.xyz / positionView.w, 1.0f), invView);
	return positionWorld.xyz / positionWorld.w;
}

#endif
