#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> SceneColorTexture;
Texture2D DirectDiffuse;
Texture2D DirectSpecular;
Texture2D DirectSubsurface;
Texture2D IndirectDiffuse;
Texture2D IndirectSpecular;
Texture2D IndirectSubsurface;

static const uint ViewModeLit = 0u;
static const uint ViewModeGBufferDiffuse = 1u;
static const uint ViewModeGBufferNormal = 2u;
static const uint ViewModeGBufferRoughness = 3u;
static const uint ViewModeGBufferMetallic = 4u;
static const uint ViewModeGBufferEmissive = 5u;
static const uint ViewModeGBufferAmbientOcclusion = 6u;
static const uint ViewModeGBufferSubsurfaceColor = 7u;
static const uint ViewModeGBufferSubsurfaceStrength = 8u;
static const uint ViewModeDirectDiffuse = 9u;
static const uint ViewModeDirectSpecular = 10u;
static const uint ViewModeDirectSubsurface = 11u;
static const uint ViewModeIndirectDiffuse = 12u;
static const uint ViewModeIndirectSpecular = 13u;
static const uint ViewModeIndirectSubsurface = 14u;

float3 PreviewScalar(float value)
{
	return saturate(value).xxx;
}

float3 PreviewNormal(float3 normalWorld)
{
	return normalize(normalWorld) * 0.5f + 0.5f;
}

float3 PreviewHdr(float3 color)
{
	const float3 safeColor = max(color, 0.0f);
	return safeColor / (1.0f + safeColor);
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (ViewModeIndex == ViewModeLit)
	{
		return;
	}

	uint width = 0;
	uint height = 0;
	SceneColorTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const int3 pixel = int3(dispatchThreadId.xy, 0);
	const GBufferData gBuffer = LoadGBuffer(dispatchThreadId.xy);
	const float3 directDiffuse = DirectDiffuse.Load(pixel).rgb;
	const float3 directSpecular = DirectSpecular.Load(pixel).rgb;
	const float3 directSubsurface = DirectSubsurface.Load(pixel).rgb;
	const float3 indirectDiffuse = IndirectDiffuse.Load(pixel).rgb;
	const float3 indirectSpecular = IndirectSpecular.Load(pixel).rgb;
	const float3 indirectSubsurface = IndirectSubsurface.Load(pixel).rgb;

	float3 outputColor = 0.0f;
	switch (ViewModeIndex)
	{
		case ViewModeGBufferDiffuse:
			outputColor = saturate(gBuffer.BaseColor);
			break;
		case ViewModeGBufferNormal:
			outputColor = PreviewNormal(gBuffer.NormalWorld);
			break;
		case ViewModeGBufferRoughness:
			outputColor = PreviewScalar(gBuffer.Roughness);
			break;
		case ViewModeGBufferMetallic:
			outputColor = PreviewScalar(gBuffer.Metallic);
			break;
		case ViewModeGBufferEmissive:
			outputColor = PreviewHdr(gBuffer.Emissive);
			break;
		case ViewModeGBufferAmbientOcclusion:
			outputColor = PreviewScalar(gBuffer.AmbientOcclusion);
			break;
		case ViewModeGBufferSubsurfaceColor:
			outputColor = saturate(gBuffer.SubsurfaceColor);
			break;
		case ViewModeGBufferSubsurfaceStrength:
			outputColor = PreviewScalar(gBuffer.SubsurfaceStrength);
			break;
		case ViewModeDirectDiffuse:
			outputColor = PreviewHdr(directDiffuse);
			break;
		case ViewModeDirectSpecular:
			outputColor = PreviewHdr(directSpecular);
			break;
		case ViewModeDirectSubsurface:
			outputColor = PreviewHdr(directSubsurface);
			break;
		case ViewModeIndirectDiffuse:
			outputColor = PreviewHdr(indirectDiffuse);
			break;
		case ViewModeIndirectSpecular:
			outputColor = PreviewHdr(indirectSpecular);
			break;
		case ViewModeIndirectSubsurface:
			outputColor = PreviewHdr(indirectSubsurface);
			break;
		default:
			return;
	}

	SceneColorTexture[dispatchThreadId.xy] = float4(outputColor, gBuffer.Alpha);
}