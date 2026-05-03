#ifndef SPARKLE_GBUFFER_UTILS_HLSLI
#define SPARKLE_GBUFFER_UTILS_HLSLI

Texture2D GBufferBaseColor;
Texture2D GBufferNormal;
Texture2D GBufferMaterial;
Texture2D GBufferEmissive;

struct GBufferData
{
	float3 BaseColor;
	float Alpha;
	float3 NormalWorld;
	float Metallic;
	float Roughness;
	float AmbientOcclusion;
	float MaterialFlags;
	float3 Emissive;
};

float3 DecodeGBufferNormal(float3 normalWorld)
{
	const float lengthSquared = dot(normalWorld, normalWorld);
	return lengthSquared > 0.0f ? normalWorld * rsqrt(lengthSquared) : float3(0.0f, 0.0f, 1.0f);
}

GBufferData LoadGBuffer(uint2 pixelCoord)
{
	const int3 pixel = int3(pixelCoord, 0);
	const float4 baseColorSample = GBufferBaseColor.Load(pixel);
	const float4 normalSample = GBufferNormal.Load(pixel);
	const float4 materialSample = GBufferMaterial.Load(pixel);
	const float4 emissiveSample = GBufferEmissive.Load(pixel);

	GBufferData gBuffer;
	gBuffer.BaseColor = saturate(baseColorSample.rgb);
	gBuffer.Alpha = baseColorSample.a;
	gBuffer.NormalWorld = DecodeGBufferNormal(normalSample.xyz);
	gBuffer.Metallic = saturate(materialSample.r);
	gBuffer.Roughness = saturate(materialSample.g);
	gBuffer.AmbientOcclusion = saturate(materialSample.b);
	gBuffer.MaterialFlags = materialSample.a;
	gBuffer.Emissive = max(emissiveSample.rgb, 0.0f);
	return gBuffer;
}

#endif